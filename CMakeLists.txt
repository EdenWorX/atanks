# SPDX-License-Identifier: GPL-3.0-or-later
# See LICENSE. Single source of truth for the version is project(VERSION ...)
# below (cleanup task TODO.md, WP PF-1.9).

cmake_minimum_required(VERSION 3.25)

project(atanks VERSION 6.7 LANGUAGES CXX)

# The legacy Makefile only wraps cmake+ninja (TODO.md, WP PF-1.9), so the
# Ninja generator is mandatory. Windows builds keep using vs12/vs14.
if(NOT CMAKE_GENERATOR MATCHES "Ninja")
  message(FATAL_ERROR "atanks requires the Ninja generator (-G Ninja). "
    "Windows builds use vs12/atanks.sln or vs14/atanks.sln instead.")
endif()

# Compile commands for clang-tidy and other tools (WP PF-1.12).
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options mirroring the legacy Makefile knobs. UBUNTU is intentionally not
# ported: the workaround is ancient and is deleted in WP PF-1.13 instead.
option(DEBUG "Enable debug build (-Og, -DATANKS_DEBUG, debug flavors)" OFF)
option(DEBUG_AICORE "Enable AI aiming and emotion debug output" OFF)
option(DEBUG_AIMING "Enable AI aiming debug output" OFF)
option(DEBUG_EMOTION "Enable AI emotion debug output" OFF)
option(DEBUG_FINANCE "Enable finance debug output" OFF)
option(DEBUG_OBJECTS "Enable object debug output" OFF)
option(DEBUG_PHYSICS "Enable physics debug output" OFF)
option(DEBUG_LOG_TO_FILE "Write debug output to atanks.log instead of stdout" OFF)
option(SANITIZE_ADDRESS "Build with -fsanitize=address" OFF)
option(SANITIZE_THREAD "Build with -fsanitize=thread" OFF)
option(SANITIZE_UNDEF "Build with -fsanitize=undefined (combines with address/thread)" OFF)
option(USE_LTO "Enable link-time optimization (release only)" OFF)
option(GCCUSESGOLD "Use the linker plugin with LTO (requires USE_LTO)" OFF)

# Install layout equivalents of Makefile PREFIX/BINDIR/INSTALLDIR.
# DESTDIR staging works natively via `cmake --install <dir> --prefix ...`
# or the DESTDIR environment variable. ATANKS_DATA_DIR is baked into the
# generated config.h (see below).
set(ATANKS_INSTALL_BINDIR "bin" CACHE STRING "Binary install directory, relative to prefix")
set(ATANKS_DATA_DIR "${CMAKE_INSTALL_PREFIX}/share/atanks" CACHE STRING "Data directory baked into the binary")

# Platform selection mirrors the Makefile: macOS via osxuser, everything
# else Unix builds as LINUX (including BSD). No WIN32 support here.
if(APPLE)
  set(ATANKS_PLATFORM_MACOSX ON)
  set(ATANKS_WITH_NETWORK OFF)
  set(ATANKS_IS_BSD OFF)
elseif(CMAKE_SYSTEM_NAME MATCHES ".*BSD|DragonFly")
  set(ATANKS_PLATFORM_MACOSX OFF)
  set(ATANKS_WITH_NETWORK ON)
  set(ATANKS_IS_BSD ON)
else()
  set(ATANKS_PLATFORM_MACOSX OFF)
  set(ATANKS_WITH_NETWORK ON)
  set(ATANKS_IS_BSD OFF)
endif()

# Allegro 4 is a system dependency (not vendored).
find_program(ALLEGRO_CONFIG allegro-config)
if(NOT ALLEGRO_CONFIG)
  message(FATAL_ERROR "allegro-config not found. Install the Allegro 4 development package first.")
endif()
execute_process(COMMAND ${ALLEGRO_CONFIG} --cppflags OUTPUT_VARIABLE ALLEGRO_CPPFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${ALLEGRO_CONFIG} --libs OUTPUT_VARIABLE ALLEGRO_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)
separate_arguments(ALLEGRO_CPPFLAGS)
separate_arguments(ALLEGRO_LIBS)

# Sources mirror the Makefile wildcard (sorted, auto-picked-up).
file(GLOB ATANKS_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# Generated build configuration (WP PF-1.9.2). This replaces the legacy
# -DVERSION= / -DDATA_DIR= / -DNETWORK / platform-define flag plumbing; the
# legacy Makefile keeps passing -D flags until WP PF-1.9.4. Non-CMake builds
# (Visual Studio) never define ATANKS_HAVE_CONFIG_H, so src/main.h skips the
# include there and keeps its missing-VERSION hard-error.
if(ATANKS_WITH_NETWORK)
  set(NETWORK ON)
endif()
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/config.h"
  @ONLY)

add_executable(atanks ${ATANKS_SOURCES})

# Shared build flags for the game and the unit tests below, so both compile
# with identical warnings, defines, and sanitizer settings.
add_library(atanks_buildflags INTERFACE)
target_include_directories(atanks_buildflags INTERFACE "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_definitions(atanks_buildflags INTERFACE ATANKS_HAVE_CONFIG_H)
target_compile_features(atanks_buildflags INTERFACE cxx_std_17)
target_compile_options(atanks_buildflags INTERFACE -Wall -Wextra -Wpedantic -fexceptions)
if(ATANKS_IS_BSD)
  target_include_directories(atanks_buildflags INTERFACE /usr/local/include)
  target_compile_options(atanks_buildflags INTERFACE -Wno-c99-extensions)
endif()
if(APPLE)
  target_include_directories(atanks_buildflags INTERFACE /usr/local/include)
endif()
target_compile_options(atanks_buildflags INTERFACE ${ALLEGRO_CPPFLAGS})
target_link_libraries(atanks PRIVATE atanks_buildflags)

if(ATANKS_WITH_NETWORK OR NOT APPLE)
  # Linux and BSD link pthread, libm, and the Allegro libraries.
  find_package(Threads REQUIRED)
  target_link_libraries(atanks PRIVATE Threads::Threads m ${ALLEGRO_LIBS})
else()
  target_link_libraries(atanks PRIVATE ${ALLEGRO_LIBS})
endif()

# Any sanitizer implies a debug build: the Makefile wrapper maps sanitizer
# builds to debug-based build directories (e.g. cmake-build-asan).
set(ATANKS_DEBUG_BUILD ${DEBUG})
if(SANITIZE_ADDRESS OR SANITIZE_THREAD OR SANITIZE_UNDEF)
  set(ATANKS_DEBUG_BUILD ON)
endif()

if(ATANKS_DEBUG_BUILD)
  target_compile_options(atanks_buildflags INTERFACE -ggdb -Og -fstack-protector-strong -Wunused)
  target_compile_definitions(atanks_buildflags INTERFACE ATANKS_DEBUG)
  # LTO is hard blocked for debug builds, like in the Makefile.
  set(USE_LTO OFF)
  # Sanitizers: address and thread are mutually exclusive (address wins, like
  # in the legacy Makefile); undefined combines with either of them.
  if(SANITIZE_ADDRESS AND SANITIZE_THREAD)
    message(WARNING "SANITIZE_ADDRESS and SANITIZE_THREAD are mutually exclusive; address wins.")
  endif()
  set(ATANKS_SANITIZERS "")
  if(SANITIZE_ADDRESS)
    list(APPEND ATANKS_SANITIZERS "address")
  elseif(SANITIZE_THREAD)
    list(APPEND ATANKS_SANITIZERS "thread")
    target_compile_definitions(atanks_buildflags INTERFACE USE_MUTEX_INSTEAD_OF_SPINLOCK)
    target_compile_options(atanks_buildflags INTERFACE -fPIC -O2 -ggdb)
    target_link_options(atanks_buildflags INTERFACE -pie -O2 -ggdb)
  endif()
  if(SANITIZE_UNDEF)
    list(APPEND ATANKS_SANITIZERS "undefined")
  endif()
  if(ATANKS_SANITIZERS)
    string(JOIN "," ATANKS_SANITIZER_LIST ${ATANKS_SANITIZERS})
    target_compile_options(atanks_buildflags INTERFACE "-fsanitize=${ATANKS_SANITIZER_LIST}")
    target_link_options(atanks_buildflags INTERFACE "-fsanitize=${ATANKS_SANITIZER_LIST}")
  endif()
  if(DEBUG_AICORE)
    target_compile_definitions(atanks_buildflags INTERFACE ATANKS_DEBUG_AIMING ATANKS_DEBUG_EMOTIONS)
  endif()
  if(DEBUG_AIMING)
    target_compile_definitions(atanks_buildflags INTERFACE ATANKS_DEBUG_AIMING)
  endif()
  if(DEBUG_EMOTION)
    target_compile_definitions(atanks_buildflags INTERFACE ATANKS_DEBUG_EMOTIONS)
  endif()
  if(DEBUG_FINANCE)
    target_compile_definitions(atanks_buildflags INTERFACE ATANKS_DEBUG_FINANCE)
  endif()
  if(DEBUG_OBJECTS)
    target_compile_definitions(atanks_buildflags INTERFACE ATANKS_DEBUG_OBJECTS)
  endif()
  if(DEBUG_PHYSICS)
    target_compile_definitions(atanks_buildflags INTERFACE ATANKS_DEBUG_PHYSICS)
  endif()
  if(DEBUG_LOG_TO_FILE)
    target_compile_definitions(atanks_buildflags INTERFACE ATANKS_DEBUG_LOGTOFILE)
  endif()
else()
  target_compile_options(atanks_buildflags INTERFACE -O2)
  # Same portability trade-off as the Makefile (-march=native).
  target_compile_options(atanks_buildflags INTERFACE -march=native)
  if(USE_LTO)
    target_compile_options(atanks_buildflags INTERFACE -flto)
    if(GCCUSESGOLD)
      target_compile_options(atanks_buildflags INTERFACE -fuse-linker-plugin)
    endif()
  endif()
endif()

# Install rules mirroring the Makefile install target. (Distribution, doc, and
# analysis targets arrive in WP PF-1.9.4 and WP PF-1.12.)

# Unit tests (WP PF-1.11). The suite covers decoupled logic units only (no
# Allegro or global-state references): random, zbuffer, spinlock,
# optiontypes, box, clock, globaltypes.
include(CTest)
if(BUILD_TESTING)
  # CppUTest resolution, online only as a last resort: a local install
  # first (pkg-config probe, then CMake package config), pinned FetchContent
  # (release tag v4.0) third. The probe order is deliberate: a broken
  # installed CMake config fails with message(FATAL_ERROR), which QUIET
  # cannot suppress, while pkg_check_modules fails softly. Fully offline
  # without a system CppUTest 4.0+: configure with -DBUILD_TESTING=OFF to
  # skip the suite.
  set(ATANK_HAVE_CPPUTEST OFF)
  find_package(PkgConfig QUIET)
  if(PkgConfig_FOUND)
    pkg_check_modules(CPPUTEST IMPORTED_TARGET cpputest>=4.0)
  endif()
  if(CPPUTEST_FOUND)
    set(ATANK_CPPUTEST_LIBS PkgConfig::CPPUTEST)
    set(ATANK_HAVE_CPPUTEST ON)
  else()
    find_package(CppUTest 4.0 QUIET)
    if(CppUTest_FOUND)
      set(ATANK_CPPUTEST_LIBS CppUTest CppUTestExt)
      set(ATANK_HAVE_CPPUTEST ON)
    else()
      include(FetchContent)
      # Keep CppUTest's own suite out of our ctest run.
      set(TESTS OFF CACHE BOOL "" FORCE)
      FetchContent_Declare(CppUTest
        GIT_REPOSITORY https://github.com/cpputest/cpputest.git
        GIT_TAG v4.0)
      FetchContent_MakeAvailable(CppUTest)
      set(ATANK_CPPUTEST_LIBS CppUTest CppUTestExt)
      set(ATANK_HAVE_CPPUTEST ON)
    endif()
  endif()

  if(ATANK_HAVE_CPPUTEST)
    add_executable(atanks_tests
      tests/tests_main.cpp
      tests/test_random.cpp
      tests/test_zbuffer.cpp
      tests/test_spinlock.cpp
      tests/test_optiontypes.cpp
      tests/test_box.cpp
      tests/test_clock.cpp
      tests/test_globaltypes.cpp
      src/random.cpp
      src/zbuffer.cpp
      src/spinlock.cpp
      src/optiontypes.cpp
      src/box.cpp
      src/clock.cpp
      src/globaltypes.cpp)
    target_include_directories(atanks_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_compile_features(atanks_tests PRIVATE cxx_std_17)
    target_link_libraries(atanks_tests PRIVATE atanks_buildflags ${ATANK_CPPUTEST_LIBS})
    add_test(NAME units COMMAND atanks_tests)
  endif()
endif()

# Documentation (WP PF-1.12). `make doc` builds the public API reference from
# the headers into <builddir>/doc; undocumented public APIs fail the build so
# coverage gaps stay visible. Plain builds never need Doxygen, so the
# dependency stays optional and only the doc target reports its absence.
find_package(Doxygen QUIET)
# Doxygen appends its own html/ subdir, so the output directory is docs/ and
# the reference lands in docs/html/ (ignored build output, see .gitignore).
set(DOXYGEN_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/docs")
set(DOXYGEN_WARN_LOG "${CMAKE_CURRENT_BINARY_DIR}/doc-warnings.log")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in"
  "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
  @ONLY)
if(DOXYGEN_FOUND)
  set(ATANK_DOC_WARN_LOG "${DOXYGEN_WARN_LOG}")
  add_custom_target(doc
    COMMAND ${CMAKE_COMMAND} -E rm -f "${ATANK_DOC_WARN_LOG}"
    COMMAND ${DOXYGEN_EXECUTABLE} "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
    COMMAND ${CMAKE_COMMAND} -DDOC_WARN_LOG=${ATANK_DOC_WARN_LOG}
      -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/doc-fail-on-warnings.cmake"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM)
else()
  add_custom_target(doc
    COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/doc-missing.cmake"
    COMMENT "Doxygen not found")
endif()

install(TARGETS atanks RUNTIME DESTINATION ${ATANKS_INSTALL_BINDIR}
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
install(FILES io.github.EdenWorX.atanks.metainfo.xml DESTINATION share/metainfo)
install(FILES atanks.desktop DESTINATION share/applications)
install(FILES atanks.png DESTINATION share/icons/hicolor/48x48/apps)
file(GLOB ATANKS_DATA_TOPFILES
  "${CMAKE_CURRENT_SOURCE_DIR}/COPYING"
  "${CMAKE_CURRENT_SOURCE_DIR}/README"
  "${CMAKE_CURRENT_SOURCE_DIR}/TODO"
  "${CMAKE_CURRENT_SOURCE_DIR}/docs/Changelog.history"
  "${CMAKE_CURRENT_SOURCE_DIR}/*.txt"
  "${CMAKE_CURRENT_SOURCE_DIR}/unicode.dat")
# The build file itself matches *.txt; it is not data.
list(REMOVE_ITEM ATANKS_DATA_TOPFILES "${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt")
install(FILES ${ATANKS_DATA_TOPFILES} DESTINATION share/atanks)
foreach(ATANK_ASSET_DIR button misc missile sound stock tank tankgun title text)
  install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${ATANK_ASSET_DIR}/" DESTINATION share/atanks/${ATANK_ASSET_DIR})
endforeach()
