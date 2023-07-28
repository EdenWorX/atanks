#ifndef FILE_HANDLING_HEADER_
#define FILE_HANDLING_HEADER_

#define MAX_CONFIG_LINE 128


#include "debug.h"
#include "environment.h"
#include "globaldata.h"
#include "text.h"
#include "wrap_dirent.h"


bool Save_Game();
bool Load_Game();
bool Check_For_Saved_Game();
bool Copy_Config_File();


// Make sure there is a music folder in .atanks
bool Create_Music_Folder();
void scrollTextList( TEXTBLOCK* lines );
void flush_inputs();
bool Load_Weapons_Text();


#ifdef MACOSX
int Filter_File( struct dirent* my_file );
#else
int Filter_File( const struct dirent* my_file );
#endif

dirent** Find_Saved_Games( uint32_t& num_files_found );

char**   Find_Bitmaps( int32_t* bitmaps_found );

#endif
