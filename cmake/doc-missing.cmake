# Fails the `doc` target with a helpful message when Doxygen is absent (WP PF-1.12).
# Plain builds never need Doxygen, so the dependency stays optional at configure time.
message(FATAL_ERROR "Doxygen not found. Install doxygen, then re-run make doc.")
