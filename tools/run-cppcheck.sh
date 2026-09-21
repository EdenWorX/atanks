#!/bin/bash
# Static analysis runner for atanks (TODO.md, WP PF-1.12).
#
# Usage: bash tools/run-cppcheck.sh [build-dir]
#
# Checks src/ and tests/ with the project's include paths and C++17
# standard. Needs a configured CMake build directory for the generated
# config.h (default: ./cmake-build-release); configure one first with e.g.
# `cmake -S . -B cmake-build-release -G Ninja`.
# Exits non-zero when cppcheck reports an error, so CI can gate on it.
# Run it before committing changes that touch C++ sources or headers.
#
# Triaged findings (accepted, not fixed — actionable ones live as issues in
# TODO_Xtra.md instead). Accepted style, no action: cstyleCast,
# dangerousTypeCast, noCopyConstructor, noOperatorEq, useStlAlgorithm,
# constVariablePointer, constParameterPointer, constParameterReference,
# variableScope (except the tank.cpp/shop.cpp instances in TODO_Xtra.md),
# functionStatic, redundantAssignment, redundantInitialization,
# uselessOverride, clarifyCalculation, duplicateAssignExpression.
# Accepted false positives, with reasoning: invalidPrintfArgType_uint for %lu
# with size_t in src/weapon.cpp (identical types on LP64; revisit for LLP64
# Windows work); unknownMacro for END_OF_FUNCTION in src/winclock.h (needs
# MSVC configuration, Linux parse noise); bufferAccessOutOfBounds in
# src/network.cpp Send_Message (strncpy with n == sizeof dst plus explicit
# NUL at [MAX_MESSAGE_LENGTH - 1]); arrayIndexOutOfBoundsCond in
# src/globaldata.cpp get_random_tank (index invariant 0..MAXPLAYERS-1 holds:
# entry range plus wrap reset); nullPointerRedundantCheck (defensive checks);
# duplicateCondition in src/levelcreator.cpp (can_work reads a cross-thread
# flag, so the re-check is intentional); knownConditionTrueFalse hits that
# are regression checks by intent carry // cppcheck-suppress comments at the
# site instead. Genuinely suspicious findings (missile null dereference,
# unread variables, teleport conditions, duplicate assignments) need
# gameplay-context review and are tracked as issues in TODO_Xtra.md.

set -u

BUILD_DIR="${1:-cmake-build-release}"

if ! command -v cppcheck >/dev/null; then
	echo "ERROR: cppcheck not found. Install it first." >&2
	exit 2
fi

if [ ! -f "${BUILD_DIR}/config.h" ]; then
	echo "ERROR: ${BUILD_DIR}/config.h not found." >&2
	echo "Configure a build directory first, e.g.:" >&2
	echo "  cmake -S . -B ${BUILD_DIR} -G Ninja" >&2
	exit 2
fi

exec cppcheck \
	--enable=warning,style,performance,portability \
	--std=c++17 \
	--force \
	--inline-suppr \
	--error-exitcode=1 \
	-I src \
	-I "${BUILD_DIR}" \
	-DATANKS_HAVE_CONFIG_H \
	src/ tests/
