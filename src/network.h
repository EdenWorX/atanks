#ifndef NETWORK_HEADER_FILE__
#define NETWORK_HEADER_FILE__

/*
This file will contain two sets of headers and data. One for dealing with queued message
and the other for handling server sockets. This queue will be in standard, platform-neutral
C++. However, the sockets will use Linux/UNIX/BSD specific code, which will have to be
updated to run on other operating systems.
-- Jesse
*/


#ifndef TRUE
#  define TRUE 1
#endif
#ifndef FALSE
#  define FALSE 0
#endif

#define MAX_MESSAGE_LENGTH 256

struct MESSAGE {
	char *text;
	int   to; // which client does the message go to? May not be used as most will go to everyone
	void *next;
};

class MESSAGE_QUEUE {
public:
	MESSAGE *first_message, *last_message;

	MESSAGE_QUEUE();
	~MESSAGE_QUEUE();

	// add a message to the queue
	bool Add( char *some_text, int to );

	// pull the first message from the queue and erase it from the queue
	MESSAGE *Read();

	// read the next message in the queue without erasing it
	MESSAGE *Peek();

	MESSAGE *Read_To( int to );

	// erases the next message in the queue without reading it
	void Erase();

	// erase all messages in the queue
	void Erase_All();
};

struct SEND_RECEIVE_TYPE {
	int listening_port;
	int shut_down;
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

int      Setup_Server_Socket( int port );
int      Setup_Client_Socket( char *server, char *port );
int      Accept_Incoming_Connection( int my_socket );
int      Send_Message( MESSAGE *mess, int to_socket );
MESSAGE *Receive_Message( int from_socket );
void     Clean_Up_Server_Socket( int my_socket );
void     Clean_Up_Client_Socket( int my_socket );
int      Check_For_Incoming_Data( int socket_number );
int      Check_For_Errors( int socket_number );
void    *Send_And_Receive( void *data_we_need );

#else
#  define SAFE_WRITE( sock_, fmt_, ... ) \
	  {}
#endif


#endif
