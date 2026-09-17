#ifndef ATANKS_WRAP_DIRENT_H
#define ATANKS_WRAP_DIRENT_H 1
//
// Created by sed on 27.07.23.
//

#include "debug.h"

#ifndef HAS_DIRENT
#  if defined( ATANKS_IS_MSVC )
#    include "extern/dirent.h"
#  else
#    include <dirent.h>
#  endif // Linux
#  define HAS_DIRENT 1
#endif // HAS_DIRENT


#endif // ATANKS_WRAP_DIRENT_H
