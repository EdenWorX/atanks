#include "update.h"

#include "externs.h"
#include "network.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <sys/types.h>

#if defined( ATANKS_IS_MSVC )
/// What is needed here?
#else
#  include <netdb.h>
#  include <unistd.h>

#  include <netinet/in.h>
#  include <sys/socket.h>
#endif // MSVC++ versus gcc/clang

/// @brief UpdateData default ctor
UpdateData::UpdateData( char const* server_, char const* remote_, char const* host_ )
	: server_name( server_ ? strdup( server_ ) : strdup( "" ) )
	, host_name( host_ ? strdup( host_ ) : strdup( "" ) )
	, remote_file( remote_ ? strdup( remote_ ) : strdup( "" ) ) {}

/// @brief UpdateData default dtor
UpdateData::~UpdateData() {
	if ( server_name ) {
		free( server_name );
	}
	if ( host_name ) {
		free( host_name );
	}
	if ( remote_file ) {
		free( remote_file );
	}
}

void UpdateData::operator() () {
#ifdef NETWORK
	if ( env.check_for_updates ) {
		// set up socket
		int                socket_num, port_number = 80;

		struct sockaddr_in server_address {};
		struct hostent*    server;
		char               buffer[ 1024 ];
		char*              found = nullptr;
		int                got_bytes;


		socket_num = socket( AF_INET, SOCK_STREAM, 0 );
		if ( socket_num < 0 ) {
			return;
		}

		if ( global.is_close_btn_pressed() ) {
			goto getout;
		}

		server = gethostbyname( server_name );
		if ( !server ) {
			goto getout;
		}

		if ( global.is_close_btn_pressed() ) {
			goto getout;
		}

		bzero( (char*)&server_address, sizeof( server_address ) );
		server_address.sin_family = AF_INET;
		bcopy( (char*)server->h_addr, (char*)&server_address.sin_addr.s_addr, server->h_length );
		server_address.sin_port = htons( port_number );

		// try to connect
		if ( connect( socket_num, (sockaddr*)&server_address, sizeof( server_address ) ) < 0 ) {
			goto getout;
		}

		if ( global.is_close_btn_pressed() ) {
			goto getout;
		}

		// get HTTP data
		SAFE_WRITE( socket_num, "GET /%s HTTP/1.1\nHost: %s\n\n", remote_file, host_name );

		got_bytes = static_cast< int >( read( socket_num, buffer, 1024 ) );

		if ( global.is_close_btn_pressed() ) {
			goto getout;
		}

		// search for version number in return data
		if ( got_bytes > 1 ) {
			found = strstr( buffer, "Version: " );
		}
		while ( ( got_bytes > 1 ) && ( !found ) ) {
			got_bytes = static_cast< int >( read( socket_num, buffer, 1024 ) );
			if ( got_bytes > 1 ) {
				found = strstr( buffer, "Version: " );
			}
		}

		// compare version number
		if ( found ) {
			found              += 9;
			found[ 5 ]          = '\0';

			double web_version  = 0.;
			SAFE_STOD( web_version, found );

			auto ext_version = static_cast< int32_t >( web_version * 10 );

			if ( ext_version > game_version ) {
				snprintf( update_string, 1024, "A new version, %2.1lf, is ready for download.", web_version );
			}
		}
getout:
		close( socket_num );
	}
#endif // NETWORK
}
