#ifndef ATANKS_NETWORK_H_INCLUDED
#define ATANKS_NETWORK_H_INCLUDED 1

// Build configuration (CMake only): the NETWORK macro lives in the generated
// config.h, which must be visible before the first #ifdef NETWORK below
// regardless of include order. Non-CMake builds keep passing -DNETWORK= on
// the compiler command line, so this include stays conditional.
#ifdef ATANKS_HAVE_CONFIG_H
#  include "config.h"
#endif

/*
This file will contain two sets of headers and data. One for dealing with queued message
and the other for handling server sockets. This queue will be in standard, platform-neutral
C++. However, the sockets will use Linux/UNIX/BSD specific code, which will have to be
updated to run on other operating systems.
-- Jesse
*/


#define MAX_MESSAGE_LENGTH 256

/** @struct sMessage
 * @brief Queued network message.
 **/
struct sMessage {
	char *text; ///< Message text.
	int   to;   ///< Receiving client.
	void *next; ///< Next queued message.
};

/** @class CMessageQueue
 * @brief FIFO queue of network messages.
 **/
class CMessageQueue {
public:
	sMessage *first_message; ///< Queue head.
	sMessage *last_message;  ///< Queue tail.

	/// Construct an empty queue.
	CMessageQueue();
	/// Destroy the queue.
	~CMessageQueue();

	/// add a message to the queue.
	bool add( char *some_text, int to );

	/// Pull the first message from the queue and erase it from the queue.
	sMessage *read();

	/// read the next message in the queue without erasing it.
	[[nodiscard]] sMessage *peek() const;

	/// read the next message for a client.
	[[nodiscard]] sMessage *read_to( int to );

	/// erase the next message in the queue without reading it.
	void erase();

	/// erase all messages in the queue.
	void erase_all();
};

/** @struct sSendReceive
 * @brief Network thread control block.
 **/
struct sSendReceive {
	int  listening_port; ///< Server listen port.
	bool shut_down;      ///< Stop the network thread.
};

#define DEFAULT_NETWORK_PORT 25645
#define MAX_CLIENTS          10
#define NET_COMMAND_SIZE     64
// if we do not get a command after this amount of seconds,
// turn control over to the computer for a moment
#define NET_DELAY       1000
#define NET_DELAY_SHORT 500

#ifdef NETWORK

#  include <cstdarg>
#  include <cstdint>
#  include <cstdio>
#  include <cstring>
#  include <iostream>
#  include <unistd.h>

/// Wrapper for safe socket writes with return value check.
/// A thread local char buffer is used to put the message in.
/// Please use the SAFE_WRITE macro for easier usage and automatic __FILE__ and __LINE__ setup.
static inline void safe_write_func( int sock_, char const *file_, int32_t line_, char const *fmt_, ... ) {
	static thread_local char buffer[ NET_COMMAND_SIZE ] = { 0x0 };

	// Use simple one-try vsprintf(), we can safely assume no message will ever need more than 512 bytes.
	va_list args;
	va_start( args, fmt_ );
	vsprintf( buffer, fmt_, args );
	va_end( args );

	size_t  towrite = strlen( buffer );
	ssize_t written = write( sock_, buffer, towrite );

	if ( written < static_cast< ssize_t >( towrite ) ) {
		fprintf( stderr, "%s:%d: Warning: Only %zd/%zu bytes sent to server\n", file_, line_, written, towrite );
	}
}

#  define SAFE_WRITE( sock_, fmt_, ... ) safe_write_func( sock_, __FILE__, __LINE__, fmt_, __VA_ARGS__ )

int      setup_server_socket( int port );
int      setup_client_socket( char *server, char const *port );
int      accept_incoming_connection( int my_socket );
int      send_message( sMessage *mess, int to_socket );
sMessage *receive_message( int from_socket );
void     clean_up_server_socket( int my_socket );
void     clean_up_client_socket( int my_socket );
int      check_for_incoming_data( int socket_number );
bool     check_for_errors( int socket_number );
void     send_and_receive( void *data_we_need );

#else
#  define SAFE_WRITE( sock_, fmt_, ... ) \
	  {}
#endif


#endif // ATANKS_NETWORK_H_INCLUDED
