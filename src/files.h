#ifndef ATANKS_FILES_H_INCLUDED
#define ATANKS_FILES_H_INCLUDED 1


#include "debug.h"
#include "environment.h"
#include "globaldata.h"
#include "text.h"
#include "wrap_dirent.h"


/// @brief Read one full line from a config/savegame file.
/// @details Grows past any length instead of splitting like fgets() into a fixed buffer did.
/// Trailing newline characters are stripped like the old parsing did.
/// @param file Open file to read from.
/// @param line Receives the line without its newline.
/// @return false on end of file (nothing read), true otherwise.
bool read_config_line( FILE* file, string& line );

/// @brief Split a config line into field and value at the first '=' (search starts at index 1).
/// @param line Line as read by read_config_line().
/// @param field Receives the part before '='.
/// @param value Receives the part after '='.
/// @return false if there is no '=' at position >= 1, true otherwise.
bool split_config_field( const string& line, string& field, string& value );


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
