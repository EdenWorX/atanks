.PHONY: aidebug all bsduser clean debug dist fulldebug i686-dist install osxuser \
    source-dist tarball ubuntu user veryclean zipfile

# Note: Submit as "YES" to enable debugging
DEBUG   := $(if $(DEBUG),$(DEBUG),NO)

# The following switches can be used to fine-tune the debugging output:
# Note: DEBUG_AICORE can be used to enable both DEBUG_AIMING and DEBUG_EMOTION
#       together with a single flag.
DEBUG_AICORE  := $(if $(DEBUG_AICORE),$(DEBUG_AICORE),NO)
DEBUG_AIMING  := $(if $(DEBUG_AIMING),$(DEBUG_AIMING),NO)
DEBUG_EMOTION := $(if $(DEBUG_EMOTION),$(DEBUG_EMOTION),NO)
DEBUG_FINANCE := $(if $(DEBUG_FINANCE),$(DEBUG_FINANCE),NO)
DEBUG_OBJECTS := $(if $(DEBUG_OBJECTS),$(DEBUG_OBJECTS),NO)
DEBUG_PHYSICS := $(if $(DEBUG_PHYSICS),$(DEBUG_PHYSICS),NO)

# If the debug output shall be written to atanks.log, set this to YES
# ( Hint: If you enable more than one option above, you WANT to say YES here! ;-) )
DEBUG_LOG_TO_FILE := $(if $(DEBUG_LOG_TO_FILE),$(DEBUG_LOG_TO_FILE),NO)

# Address and thread sanitizers are mutually exclusive (address wins);
# undefined combines with either. Any sanitizer implies a debug build.
# SANITIZE_LEAK was dropped: lsan is part of asan now.
SANITIZE_ADDRESS := $(if $(SANITIZE_ADDRESS),$(SANITIZE_ADDRESS),NO)
SANITIZE_THREAD  := $(if $(SANITIZE_THREAD),$(SANITIZE_THREAD),NO)
SANITIZE_UNDEF   := $(if $(SANITIZE_UNDEF),$(SANITIZE_UNDEF),NO)

ifeq (YES,$(SANITIZE_LEAK))
  $(warning SANITIZE_LEAK was dropped because lsan is part of asan now; use SANITIZE_ADDRESS=YES instead)
endif

# The following is only used without debugging enabled.
USE_LTO     := $(if $(USE_LTO),$(USE_LTO),NO)

# Set to one if wanting to use linker plugins. Requires USE_LOT to be YES
GCCUSESGOLD := $(if $(GCCUSESGOLD),$(GCCUSESGOLD),NO)


# -----------------------------------------------------------------------------------------------------------------------------
# Install and target directories
# -----------------------------------------------------------------------------------------------------------------------------
PREFIX     := $(if $(PREFIX),$(PREFIX),/usr)
DESTDIR    := $(if $(DESTDIR),$(DESTDIR),)
BINPREFIX  := $(if $(BINPREFIX),$(BINPREFIX),$(PREFIX))
BINDIR     := $(if $(BINDIR),$(BINDIR),${BINPREFIX}/bin)
INSTALLDIR := $(if $(INSTALLDIR),$(INSTALLDIR),${PREFIX}/share/atanks)


# If this is a user make goal, the install directory is forced to be local:
ifneq (,$(findstring user,$(MAKECMDGOALS)))
  INSTALLDIR := .
endif


# -----------------------------------------------------------------------------------------------------------------------------
# Version single source of truth: project(VERSION ...) in CMakeLists.txt.
# -----------------------------------------------------------------------------------------------------------------------------
VERSION := $(shell grep '^project.atanks VERSION' CMakeLists.txt | grep -o '[0-9][0-9.]*' | head -n 1)


# -----------------------------------------------------------------------------------------------------------------------------
# Target executable and distribution file name
# -----------------------------------------------------------------------------------------------------------------------------
TARGET   := atanks
FILENAME := $(TARGET)-$(VERSION)


# -----------------------------------------------------------------------------------------------------------------------------
# Tools to use (thin cmake+ninja wrapper)
# -----------------------------------------------------------------------------------------------------------------------------
CMAKE := cmake
MAKE  := $(shell which make)
RM    := $(shell which rm) -f


# -----------------------------------------------------------------------------------------------------------------------------
# Build directory owned by the wrapper
# -----------------------------------------------------------------------------------------------------------------------------
# Base ./cmake-build plus option postfixes (TODO.md, WP PF-1.9.4):
# DEBUG=NO -> -release, DEBUG=YES -> -debug,
# SANITIZE_ADDRESS=YES -> -asan, SANITIZE_THREAD=YES -> -tsan,
# SANITIZE_UNDEF=YES appends -usan. Any sanitizer implies DEBUG=YES,
# so no extra -debug postfix is added (bare SANITIZE_UNDEF=YES gives
# -usan, SANITIZE_ADDRESS=YES SANITIZE_UNDEF=YES gives -asan-usan).
BUILDDIR_BASE := cmake-build
BUILD_SUFFIX  := -release
ifeq (YES,$(DEBUG))
  BUILD_SUFFIX := -debug
endif
ifeq (YES,$(SANITIZE_ADDRESS))
  BUILD_SUFFIX := -asan
else ifeq (YES,$(SANITIZE_THREAD))
  BUILD_SUFFIX := -tsan
endif
ifeq (YES,$(SANITIZE_UNDEF))
  ifneq (,$(filter -release -debug,$(BUILD_SUFFIX)))
    BUILD_SUFFIX := -usan
  else
    BUILD_SUFFIX := $(BUILD_SUFFIX)-usan
  endif
endif
BUILDDIR := $(BUILDDIR_BASE)$(BUILD_SUFFIX)

# BINDIR relative to PREFIX for -DATANKS_INSTALL_BINDIR.
BINDIR_REL := $(patsubst $(PREFIX)/%,%,$(BINDIR))

# Flags forwarded to the CMake configure step.
CMAKE_FLAGS := -G Ninja -DCMAKE_INSTALL_PREFIX=$(PREFIX) -DATANKS_DATA_DIR=$(INSTALLDIR)
CMAKE_FLAGS := ${CMAKE_FLAGS} -DATANKS_INSTALL_BINDIR=$(BINDIR_REL)
CMAKE_FLAGS := ${CMAKE_FLAGS} -DDEBUG=$(DEBUG) -DDEBUG_AICORE=$(DEBUG_AICORE)
CMAKE_FLAGS := ${CMAKE_FLAGS} -DDEBUG_AIMING=$(DEBUG_AIMING) -DDEBUG_EMOTION=$(DEBUG_EMOTION)
CMAKE_FLAGS := ${CMAKE_FLAGS} -DDEBUG_FINANCE=$(DEBUG_FINANCE) -DDEBUG_OBJECTS=$(DEBUG_OBJECTS)
CMAKE_FLAGS := ${CMAKE_FLAGS} -DDEBUG_PHYSICS=$(DEBUG_PHYSICS) -DDEBUG_LOG_TO_FILE=$(DEBUG_LOG_TO_FILE)
CMAKE_FLAGS := ${CMAKE_FLAGS} -DSANITIZE_ADDRESS=$(SANITIZE_ADDRESS) -DSANITIZE_THREAD=$(SANITIZE_THREAD)
CMAKE_FLAGS := ${CMAKE_FLAGS} -DSANITIZE_UNDEF=$(SANITIZE_UNDEF)
CMAKE_FLAGS := ${CMAKE_FLAGS} -DUSE_LTO=$(USE_LTO) -DGCCUSESGOLD=$(GCCUSESGOLD)

# Built binary inside the configured tree (used by the dist targets).
BUILDBINARY := $(BUILDDIR)/atanks


# -----------------------------------------------------------------------------------------------------------------------------
# Default target
# -----------------------------------------------------------------------------------------------------------------------------

all: configure
	$(CMAKE) --build $(BUILDDIR)

# Always reconfigure: this keeps flag changes from going stale when the
# same build directory is reused with different options.
.PHONY: configure
configure: CMakeLists.txt config.h.in
	$(CMAKE) -S . -B $(BUILDDIR) $(CMAKE_FLAGS)


# -----------------------------------------------------------------------------------------------------------------------------
# User (local) targets
# -----------------------------------------------------------------------------------------------------------------------------

user osxuser bsduser: all

# The legacy -DUBUNTU workaround is not carried over to the CMake build
# (it is deleted everywhere in WP PF-1.13); ubuntu builds the default
# configuration meanwhile.
ubuntu:
	@echo "NOTE: the -DUBUNTU workaround is not part of the CMake build and will be removed in WP PF-1.13."
	@$(MAKE) -f Makefile all


# -----------------------------------------------------------------------------------------------------------------------------
# Debugging targets
# -----------------------------------------------------------------------------------------------------------------------------

aidebug:
	$(MAKE) -f Makefile DEBUG=YES DEBUG_AICORE=YES DEBUG_LOG_TO_FILE=YES

debug:
	$(MAKE) -f Makefile DEBUG=YES DEBUG_LOG_TO_FILE=YES

fulldebug:
	$(MAKE) -f Makefile DEBUG=YES DEBUG_AICORE=YES DEBUG_FINANCE=YES DEBUG_OBJECTS=YES DEBUG_PHYSICS=YES DEBUG_LOG_TO_FILE=YES


# -----------------------------------------------------------------------------------------------------------------------------
# Regular targets
# -----------------------------------------------------------------------------------------------------------------------------
install: all
	DESTDIR=$(DESTDIR) $(CMAKE) --install $(BUILDDIR) --prefix $(PREFIX)

clean:
	$(RM) -r cmake-build*
	$(RM) obj/* atanks

veryclean: clean


# -----------------------------------------------------------------------------------------------------------------------------
# Distribution targets
# -----------------------------------------------------------------------------------------------------------------------------
# Note: DISTCOMMON below references legacy atanks/* paths that no longer
# exist; these targets share that pre-existing rot and are kept as-is
# apart from the moved build binary.

dist: source-dist i686-dist

tarball: veryclean
	cd .. && tar --create --file $(FILENAME).tar.gz --auto-compress --exclude-vcs $(FILENAME) 

zipfile: veryclean
	cd .. && zip -r $(FILENAME)-source.zip $(FILENAME) -x '*.git*'

source-dist:
	cd ../; \
	$(RM) $(FILENAME).tar.gz; \
	tar czf $(FILENAME).tar.gz atanks/src/*.cpp atanks/src/*.h atanks/Makefile $(DISTCOMMON)

i686-dist: all
	cd ../; \
	$(RM) $(FILENAME)-i686-dist.tar.gz; \
	strip atanks/$(BUILDBINARY); \
	tar czf $(FILENAME)-i686-dist.tar atanks/$(BUILDBINARY) $(DISTCOMMON)


# -----------------------------------------------------------------------------------------------------------------------------
# Distribution file lists
# -----------------------------------------------------------------------------------------------------------------------------
DISTCOMMON := \
atanks/*.dat atanks/COPYING atanks/README atanks/TODO \
atanks/Changelog atanks/BUGS atanks/*.txt

# Kept for reference only: the install itself moved to CMakeLists.txt.
# WP PF-1.10 updates this list when Changelog becomes docs/Changelog.history.
INCOMMON   := COPYING README TODO Changelog *.txt unicode.dat
