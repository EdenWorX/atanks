#include "debug.h"

#if defined( ATANKS_DEBUG )

#  include <cstdarg>
#  include <ctime>
#  include <iostream>
#  include <mutex>

#  if defined( ATANKS_IS_WINDOWS ) || defined( ATANKS_DEBUG_LOGTOFILE )
#    include <cstdio>
#  endif


// Log Mutex
std::mutex log_lock;

/// @brief use with DEBUG_LOG to get debug dependent positional information for free!
void debug_log( char const* moduleName, char const* title, char const* message, ... ) {
	static thread_local char timebuf[ 26 ]{ 0x0 };
	static thread_local char xMsg[ 512 ]{ 0x0 };

	struct tm                tm_ {};

	// Create timestamp
	atanks_tzset();
	time_t t = time( nullptr );
	atanks_localtime( &tm_, &t );
	atanks_snprintf(
		timebuf,
		26,
		"%04d.%02d.%02d %02d:%02d:%02d",
		static_cast< uint16_t >( tm_.tm_year + 1900 ),
		static_cast< uint8_t >( tm_.tm_mon + 1 ),
		static_cast< uint8_t >( tm_.tm_mday ),
		static_cast< uint8_t >( tm_.tm_hour ),
		static_cast< uint8_t >( tm_.tm_min ),
		static_cast< uint8_t >( tm_.tm_sec )
	);
	timebuf[ 20 ] = 0x0;

	// Create message
	va_list vl;
	va_start( vl, message );
	vsprintf_s( xMsg, 512, message, vl );
	va_end( vl );

	log_lock.lock();

#  if defined( ATANKS_IS_WINDOWS ) || defined( ATANKS_DEBUG_LOGTOFILE )
	// Unfortunately, for everything to work right,
	// a WinApp must be created. So write the log msg
	// to atanks.log instead.
	FILE* out = fopen( "atanks.log", "a" );
	if ( out ) {
		// The output format is meant to be loadable as CSV into Excel or localc,
		// so the content can be analyzed using auto-filters.
		fprintf( out, "%s|%s|%s|%s|\n", timebuf, moduleName, title, xMsg );
		fclose( out );
	}
#  endif // MSVC or explicit logging to atanks.log
#  if !defined( ATANKS_IS_WINDOWS )
	fprintf( stdout, "%s : %s : %s : %s\n", timebuf, moduleName, title, xMsg );
#  endif // !Windows


	log_lock.unlock();
}

#endif
