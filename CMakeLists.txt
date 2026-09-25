# SPDX-License-Identifier: GPL-3.0-or-later
# See LICENSE.
# Single source of truth for the version is project(VERSION ...) below.

cmake_minimum_required( VERSION 3.25 )

project( atanks VERSION 6.7.2 LANGUAGES CXX )

# The legacy Makefile only wraps cmake+ninja, so the Ninja generator is mandatory.
# Windows builds keep using vs12/vs14 for now but should move to cmake asap.
if ( NOT CMAKE_GENERATOR MATCHES "Ninja" )
	message( FATAL_ERROR "atanks requires the Ninja generator (-G Ninja). "
		 "Windows builds use vs12/atanks.sln or vs14/atanks.sln instead." )
endif ()

# Compile commands for clang-tidy and other tools.
set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_CXX_EXTENSIONS OFF )

# Options mirroring the legacy Makefile knobs.
option( DEBUG "Enable debug build (-Og, -DATANKS_DEBUG, debug flavors)" OFF )
option( DEBUG_AICORE "Enable AI aiming and emotion debug output" OFF )
option( DEBUG_AIMING "Enable AI aiming debug output" OFF )
option( DEBUG_EMOTION "Enable AI emotion debug output" OFF )
option( DEBUG_FINANCE "Enable finance debug output" OFF )
option( DEBUG_OBJECTS "Enable object debug output" OFF )
option( DEBUG_PHYSICS "Enable physics debug output" OFF )
option( DEBUG_LOG_TO_FILE "Write debug output to atanks.log instead of stdout" OFF )
option( SANITIZE_ADDRESS "Build with -fsanitize=address" OFF )
option( SANITIZE_THREAD "Build with -fsanitize=thread" OFF )
option( SANITIZE_UNDEF "Build with -fsanitize=undefined (combines with address/thread)" OFF )
option( USE_LTO "Enable link-time optimization (release only)" OFF )
option( GCCUSESGOLD "Use the linker plugin with LTO (requires USE_LTO)" OFF )

# Install layout equivalents of Makefile PREFIX/BINDIR/INSTALLDIR.
# DESTDIR staging works natively via `cmake --install <dir> --prefix ...`
# or the DESTDIR environment variable. ATANKS_DATA_DIR is baked into the
# generated config.h (see below).
set( ATANKS_INSTALL_BINDIR "bin" CACHE STRING "Binary install directory, relative to prefix" )
set( ATANKS_DATA_DIR "${CMAKE_INSTALL_PREFIX}/share/atanks" CACHE STRING "Data directory baked into the binary" )

# Platform selection mirrors the Makefile: macOS via osxuser, everything
# else Unix builds as LINUX (including BSD). No WIN32 support here.
if ( APPLE )
	set( ATANKS_PLATFORM_MACOSX ON )
	set( ATANKS_WITH_NETWORK OFF )
	set( ATANKS_IS_BSD OFF )
elseif ( CMAKE_SYSTEM_NAME MATCHES ".*BSD|DragonFly" )
	set( ATANKS_PLATFORM_MACOSX OFF )
	set( ATANKS_WITH_NETWORK ON )
	set( ATANKS_IS_BSD ON )
else ()
	set( ATANKS_PLATFORM_MACOSX OFF )
	set( ATANKS_WITH_NETWORK ON )
	set( ATANKS_IS_BSD OFF )
endif ()

# Allegro 4 is a system dependency (not vendored).
find_program( ALLEGRO_CONFIG allegro-config )
if ( NOT ALLEGRO_CONFIG )
	message( FATAL_ERROR "allegro-config not found. Install the Allegro 4 development package first." )
endif ()
execute_process( COMMAND ${ALLEGRO_CONFIG} --cppflags OUTPUT_VARIABLE ALLEGRO_CPPFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE )
execute_process( COMMAND ${ALLEGRO_CONFIG} --libs OUTPUT_VARIABLE ALLEGRO_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE )
separate_arguments( ALLEGRO_CPPFLAGS )
separate_arguments( ALLEGRO_LIBS )

# libcurl for the GitHub release update checker (TODO-PF-2). Optional: without it the game builds and runs with the
# checker compiled out (notably the legacy Visual Studio builds, which never define ATANKS_HAVE_CONFIG_H anyway).
find_package( CURL QUIET )
if ( CURL_FOUND )
	set( ATANKS_HAVE_CURL ON )
endif ()

# Sources mirror the Makefile wildcard (sorted, auto-picked-up).
file( GLOB ATANKS_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" )

# Generated build configuration.
# This replaces the legacy -DVERSION= / -DDATA_DIR= / -DNETWORK / platform-define flag plumbing.
# Non-CMake builds (Visual Studio) never define ATANKS_HAVE_CONFIG_H, so src/main.h skips the
# include there and keeps its missing-VERSION hard-error.
if ( ATANKS_WITH_NETWORK )
	set( NETWORK ON )
endif ()
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/src/config.h.in"
	"${CMAKE_CURRENT_BINARY_DIR}/config.h"
	@ONLY )

add_executable( atanks ${ATANKS_SOURCES} )

# Shared build flags for the game and the unit tests below, so both compile
# with identical warnings, defines, and sanitizer settings.
add_library( atanks_buildflags INTERFACE )
target_include_directories( atanks_buildflags INTERFACE "${CMAKE_CURRENT_BINARY_DIR}" )
target_compile_definitions( atanks_buildflags INTERFACE ATANKS_HAVE_CONFIG_H )
target_compile_features( atanks_buildflags INTERFACE cxx_std_17 )
target_compile_options( atanks_buildflags INTERFACE -Wall -Wextra -Wpedantic -fexceptions )
if ( ATANKS_IS_BSD )
	target_include_directories( atanks_buildflags INTERFACE /usr/local/include )
	target_compile_options( atanks_buildflags INTERFACE -Wno-c99-extensions )
endif ()
if ( APPLE )
	target_include_directories( atanks_buildflags INTERFACE /usr/local/include )
endif ()
target_compile_options( atanks_buildflags INTERFACE ${ALLEGRO_CPPFLAGS} )
target_link_libraries( atanks PRIVATE atanks_buildflags )

if ( ATANKS_WITH_NETWORK OR NOT APPLE )
	# Linux and BSD link pthread, libm, and the Allegro libraries.
	find_package( Threads REQUIRED )
	target_link_libraries( atanks PRIVATE Threads::Threads m ${ALLEGRO_LIBS} )
else ()
	target_link_libraries( atanks PRIVATE ${ALLEGRO_LIBS} )
endif ()

if ( ATANKS_HAVE_CURL )
	target_link_libraries( atanks PRIVATE CURL::libcurl )
endif ()

# Any sanitizer implies a debug build: the Makefile wrapper maps sanitizer
# builds to debug-based build directories (e.g. cmake-build-asan).
set( ATANKS_DEBUG_BUILD ${DEBUG} )
if ( SANITIZE_ADDRESS OR SANITIZE_THREAD OR SANITIZE_UNDEF )
	set( ATANKS_DEBUG_BUILD ON )
endif ()

if ( ATANKS_DEBUG_BUILD )
	target_compile_options( atanks_buildflags INTERFACE -ggdb -Og -fstack-protector-strong -Wunused )
	target_compile_definitions( atanks_buildflags INTERFACE ATANKS_DEBUG )
	# LTO is hard blocked for debug builds, like in the Makefile.
	set( USE_LTO OFF )
	# Sanitizers: address and thread are mutually exclusive (address wins, like
	# in the legacy Makefile); undefined combines with either of them.
	if ( SANITIZE_ADDRESS AND SANITIZE_THREAD )
		message( WARNING "SANITIZE_ADDRESS and SANITIZE_THREAD are mutually exclusive; address wins." )
	endif ()
	set( ATANKS_SANITIZERS "" )
	if ( SANITIZE_ADDRESS )
		list( APPEND ATANKS_SANITIZERS "address" )
	elseif ( SANITIZE_THREAD )
		list( APPEND ATANKS_SANITIZERS "thread" )
		target_compile_definitions( atanks_buildflags INTERFACE USE_MUTEX_INSTEAD_OF_SPINLOCK )
		target_compile_options( atanks_buildflags INTERFACE -fPIC -O2 -ggdb )
		target_link_options( atanks_buildflags INTERFACE -pie -O2 -ggdb )
	endif ()
	if ( SANITIZE_UNDEF )
		list( APPEND ATANKS_SANITIZERS "undefined" )
	endif ()
	if ( ATANKS_SANITIZERS )
		string( JOIN "," ATANKS_SANITIZER_LIST ${ATANKS_SANITIZERS} )
		target_compile_options( atanks_buildflags INTERFACE "-fsanitize=${ATANKS_SANITIZER_LIST}" )
		target_link_options( atanks_buildflags INTERFACE "-fsanitize=${ATANKS_SANITIZER_LIST}" )
	endif ()
	if ( DEBUG_AICORE )
		target_compile_definitions( atanks_buildflags INTERFACE ATANKS_DEBUG_AIMING ATANKS_DEBUG_EMOTIONS )
	endif ()
	if ( DEBUG_AIMING )
		target_compile_definitions( atanks_buildflags INTERFACE ATANKS_DEBUG_AIMING )
	endif ()
	if ( DEBUG_EMOTION )
		target_compile_definitions( atanks_buildflags INTERFACE ATANKS_DEBUG_EMOTIONS )
	endif ()
	if ( DEBUG_FINANCE )
		target_compile_definitions( atanks_buildflags INTERFACE ATANKS_DEBUG_FINANCE )
	endif ()
	if ( DEBUG_OBJECTS )
		target_compile_definitions( atanks_buildflags INTERFACE ATANKS_DEBUG_OBJECTS )
	endif ()
	if ( DEBUG_PHYSICS )
		target_compile_definitions( atanks_buildflags INTERFACE ATANKS_DEBUG_PHYSICS )
	endif ()
	if ( DEBUG_LOG_TO_FILE )
		target_compile_definitions( atanks_buildflags INTERFACE ATANKS_DEBUG_LOGTOFILE )
	endif ()
else ()
	target_compile_options( atanks_buildflags INTERFACE -O2 )
	# Same portability trade-off as the Makefile (-march=native).
	target_compile_options( atanks_buildflags INTERFACE -march=native )
	if ( USE_LTO )
		target_compile_options( atanks_buildflags INTERFACE -flto )
		if ( GCCUSESGOLD )
			target_compile_options( atanks_buildflags INTERFACE -fuse-linker-plugin )
		endif ()
	endif ()
endif ()

# tomlplusplus for the weapons/item data. Header-only, resolved like CppUTest with online as a last resort:
# a local install first (CMake package config, then a system header probe — upstream ships no .pc file, so there is no
# pkg-config tier), pinned FetchContent (release tag v3.4.0) third. Only src/files.cpp will include the header, so include dirs
# stay off the global compile lines; the check below proves the resolved headers work (result cached, re-verified after clean).
set( ATANK_HAVE_TOMLPLUSPLUS OFF )
find_package( tomlplusplus QUIET )
find_path( TOMLPLUSPLUS_INCLUDE_DIR NAMES toml++/toml.hpp )
if ( TOMLPLUSPLUS_INCLUDE_DIR )
	if ( TARGET tomlplusplus::tomlplusplus )
		set( ATANK_TOMLPLUSPLUS_TARGET tomlplusplus::tomlplusplus )
		message( STATUS "tomlplusplus: using CMake package and system headers" )
	else ()
		add_library( AtanksTomlPP INTERFACE )
		target_include_directories( AtanksTomlPP INTERFACE "${TOMLPLUSPLUS_INCLUDE_DIR}" )
		set( ATANK_TOMLPLUSPLUS_TARGET AtanksTomlPP )
		message( STATUS "tomlplusplus: using system headers in ${TOMLPLUSPLUS_INCLUDE_DIR}" )
	endif ()
	set( ATANK_TOMLPLUSPLUS_INCLUDE_DIRS "${TOMLPLUSPLUS_INCLUDE_DIR}" )
	set( ATANK_HAVE_TOMLPLUSPLUS ON )
else ()
	include( FetchContent )
	fetchcontent_declare( tomlplusplus
			      GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
			      GIT_TAG v3.4.0 )
	fetchcontent_makeavailable( tomlplusplus )
	if ( TARGET tomlplusplus::tomlplusplus )
		set( ATANK_TOMLPLUSPLUS_TARGET tomlplusplus::tomlplusplus )
	else ()
		add_library( AtanksTomlPP INTERFACE )
		target_include_directories( AtanksTomlPP INTERFACE "${tomlplusplus_SOURCE_DIR}/include" )
		set( ATANK_TOMLPLUSPLUS_TARGET AtanksTomlPP )
	endif ()
	message( STATUS "tomlplusplus: using FetchContent v3.4.0" )
	set( ATANK_TOMLPLUSPLUS_INCLUDE_DIRS "${tomlplusplus_SOURCE_DIR}/include" )
	set( ATANK_HAVE_TOMLPLUSPLUS ON )
endif ()

# Prove the resolved headers compile (header-only: plain include dirs suffice,
# no link libraries needed). Result cached per build dir by the check module.
include( CheckCXXSourceCompiles )
set( CMAKE_REQUIRED_INCLUDES ${ATANK_TOMLPLUSPLUS_INCLUDE_DIRS} )
check_cxx_source_compiles( "#include <toml++/toml.hpp>
int main() {
	toml::table tbl;
	tbl.insert( \"answer\", 42 );
	return tbl[ \"answer\" ].value_or( 0 ) == 42 ? 0 : 1;
}
" ATANK_TOMLPLUSPLUS_WORKS )
unset( CMAKE_REQUIRED_INCLUDES )
if ( NOT ATANK_TOMLPLUSPLUS_WORKS )
	message( FATAL_ERROR "tomlplusplus headers unusable: remove the broken local install, or provide network for FetchContent" )
endif ()

# The TOML arsenal parserlives in src/files.cpp only.
if ( ATANK_HAVE_TOMLPLUSPLUS )
	target_link_libraries( atanks PRIVATE ${ATANK_TOMLPLUSPLUS_TARGET} )
endif ()

# Install rules mirroring the Makefile install target.

# Unit tests. The suite covers decoupled logic units only (no Allegro or global-state references):
# random, zbuffer, spinlock, optiontypes, box, clock, globaltypes.
include( CTest )
if ( BUILD_TESTING )
	# CppUTest resolution, online only as a last resort: a local install first (pkg-config probe, then CMake package
	# config), pinned FetchContent (release tag v4.0) third. The probe order is deliberate: a broken installed CMake
	# config fails with message(FATAL_ERROR), which QUIET cannot suppress, while pkg_check_modules fails softly.
	# Fully offline without a system CppUTest 4.0+: configure with -DBUILD_TESTING=OFF to skip the suite.
	set( ATANK_HAVE_CPPUTEST OFF )
	find_package( PkgConfig QUIET )
	if ( PkgConfig_FOUND )
		pkg_check_modules( CPPUTEST IMPORTED_TARGET cpputest>=4.0 )
	endif ()
	if ( CPPUTEST_FOUND )
		set( ATANK_CPPUTEST_LIBS PkgConfig::CPPUTEST )
		set( ATANK_HAVE_CPPUTEST ON )
	else ()
		find_package( CppUTest 4.0 QUIET )
		if ( CppUTest_FOUND )
			set( ATANK_CPPUTEST_LIBS CppUTest CppUTestExt )
			set( ATANK_HAVE_CPPUTEST ON )
		else ()
			include( FetchContent )
			# Keep CppUTest's own suite out of our ctest run.
			set( TESTS OFF CACHE BOOL "" FORCE )
			# The v4.0 pin declares cmake_minimum_required(VERSION 3.1), below the CMake 4.x floor:
			# allow the old floor narrowly around this FetchContent wiring (CMake's suggested escape).
			set( CMAKE_POLICY_VERSION_MINIMUM 3.5 )
			fetchcontent_declare( CppUTest
					      GIT_REPOSITORY https://github.com/cpputest/cpputest.git
					      GIT_TAG v4.0 )
			fetchcontent_makeavailable( CppUTest )
			unset( CMAKE_POLICY_VERSION_MINIMUM )
			set( ATANK_CPPUTEST_LIBS CppUTest CppUTestExt )
			set( ATANK_HAVE_CPPUTEST ON )
		endif ()
	endif ()

	if ( ATANK_HAVE_CPPUTEST )
		add_executable( atanks_tests
				tests/tests_main.cpp
				tests/test_random.cpp
				tests/test_zbuffer.cpp
				tests/test_spinlock.cpp
				tests/test_optiontypes.cpp
				tests/test_box.cpp
				tests/test_clock.cpp
				tests/test_globaltypes.cpp
				tests/test_update.cpp
				src/random.cpp
				src/zbuffer.cpp
				src/spinlock.cpp
				src/optiontypes.cpp
				src/box.cpp
				src/clock.cpp
				src/globaltypes.cpp )
		target_include_directories( atanks_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src" )
		target_compile_features( atanks_tests PRIVATE cxx_std_17 )
		target_link_libraries( atanks_tests PRIVATE atanks_buildflags ${ATANK_CPPUTEST_LIBS} )
		add_test( NAME units COMMAND atanks_tests )
	endif ()
endif ()

# Documentation.
# `make doc` builds the public API reference from the headers into <builddir>/doc; undocumented public APIs fail the build so
# coverage gaps stay visible. Plain builds never need Doxygen, so the dependency stays optional and only the doc target reports
# its absence.
find_package( Doxygen QUIET )
# Doxygen appends its own html/ subdir, so the output directory is docs/ and the reference lands in docs/html/ (ignored build
# output, see .gitignore).
set( DOXYGEN_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/docs" )
set( DOXYGEN_WARN_LOG "${CMAKE_CURRENT_BINARY_DIR}/doc-warnings.log" )
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in"
	"${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
	@ONLY )
if ( DOXYGEN_FOUND )
	add_custom_target( doc
			   COMMAND ${CMAKE_COMMAND} -E rm -f "${DOXYGEN_WARN_LOG}"
			   COMMAND ${DOXYGEN_EXECUTABLE} "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
			   WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
			   COMMENT "Generating API documentation with Doxygen"
			   VERBATIM )
else ()
	# Doxygen stays optional for plain builds; fail only when docs are requested.
	add_custom_target( doc
			   COMMAND ${CMAKE_COMMAND} -E echo "ERROR: Doxygen not found. Install doxygen, then re-run make doc."
			   COMMAND ${CMAKE_COMMAND} -E false
			   COMMENT "Doxygen not found" )
endif ()

install( TARGETS atanks RUNTIME DESTINATION ${ATANKS_INSTALL_BINDIR}
	 PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE )
install( FILES io.github.EdenWorX.atanks.metainfo.xml DESTINATION share/metainfo )
install( FILES atanks.desktop DESTINATION share/applications )
install( FILES atanks.png DESTINATION share/icons/hicolor/48x48/apps )
file( GLOB ATANKS_DATA_TOPFILES
      "${CMAKE_CURRENT_SOURCE_DIR}/COPYING"
      "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
      "${CMAKE_CURRENT_SOURCE_DIR}/docs/Changelog.history"
      "${CMAKE_CURRENT_SOURCE_DIR}/*.txt"
      "${CMAKE_CURRENT_SOURCE_DIR}/unicode.dat" )
# The build file itself matches *.txt; it is not data.
list( REMOVE_ITEM ATANKS_DATA_TOPFILES "${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt" )
install( FILES ${ATANKS_DATA_TOPFILES} DESTINATION share/atanks )
foreach ( ATANK_ASSET_DIR button misc missile sound stock tank tankgun title text )
	install( DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${ATANK_ASSET_DIR}/" DESTINATION share/atanks/${ATANK_ASSET_DIR} )
endforeach ()

# ----------------------------------------------------------------
# --- Generate a local .clangd for clangd/LSP users (gitignored) ---
# ----------------------------------------------------------------
# The generated file reflects the build directory that configured last, so  clangd gets the correct include paths, including any
# generated headers. It is written into the source tree and stays gitignored.
if ( CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME )
	set( ATANKS_CLANGD_ADD_FLAGS "" )
	string( APPEND ATANKS_CLANGD_ADD_FLAGS "    -I${CMAKE_BINARY_DIR}/,\n" )
	string( APPEND ATANKS_CLANGD_ADD_FLAGS "    -I${CMAKE_SOURCE_DIR}/src,\n" )
	if ( ATANK_TOMLPLUSPLUS_INCLUDE_DIRS )
		foreach ( ATANK_TOML_DIR IN LISTS ATANK_TOMLPLUSPLUS_INCLUDE_DIRS )
			string( APPEND ATANKS_CLANGD_ADD_FLAGS "    -I${ATANK_TOML_DIR}/,\n" )
		endforeach ()
	elseif ( TOMLPLUSPLUS_INCLUDE_DIR AND NOT TOMLPLUSPLUS_INCLUDE_DIR MATCHES "NOTFOUND" )
		string( APPEND ATANKS_CLANGD_ADD_FLAGS "    -I${TOMLPLUSPLUS_INCLUDE_DIR}/,\n" )
	endif ()

	configure_file( .clangd.in "${CMAKE_SOURCE_DIR}/.clangd" @ONLY )
	message( STATUS "Generated ${CMAKE_SOURCE_DIR}/.clangd for clangd/LSP" )
endif ()
