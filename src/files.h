#ifndef ATANKS_FILES_H_INCLUDED
#define ATANKS_FILES_H_INCLUDED 1


// Maximum numbers supported in configuration files
// @todo : make this variable, hard-coded maximum numbers are very 90s.
#define MAX_CONFIG_LINE 128


#include "debug.h"
#include "environment.h"
#include "globaldata.h"
#include "text.h"
#include "wrap_dirent.h"


bool save_game();
bool load_game();
bool check_for_saved_game();
bool copy_config_file();


// Make sure there is a music folder in .atanks
bool create_music_folder();
void scroll_text_list( TEXTBLOCK* lines );
void flush_inputs();
bool load_weapons_text();


#ifdef MACOSX
int filter_file( struct dirent* my_file );
#else
int filter_file( const struct dirent* my_file );
#endif

dirent** find_saved_games( uint32_t& num_files_found );

char**   find_bitmaps( int32_t* bitmaps_found );

#endif // ATANKS_FILES_H_INCLUDED
