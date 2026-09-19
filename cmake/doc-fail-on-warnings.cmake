# Fails the `doc` target when Doxygen reported warnings (WP PF-1.12).
# DOC_WARN_LOG is passed by the `doc` custom target in CMakeLists.txt.
if(NOT EXISTS "${DOC_WARN_LOG}")
  message(FATAL_ERROR "Doxygen warning log missing: ${DOC_WARN_LOG}")
endif()
file(READ "${DOC_WARN_LOG}" ATANKS_DOC_WARNINGS)
if(ATANKS_DOC_WARNINGS)
  message(FATAL_ERROR "Doxygen reported warnings (see ${DOC_WARN_LOG})")
endif()
