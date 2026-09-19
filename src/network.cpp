#include "network.h"

#include "globaldata.h"
#include "player.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef NETWORK
#  include <sys/select.h>
#  include <sys/socket.h>
#  include <sys/types.h>
#  if defined( ATANKS_IS_BSD )
#    include <arpa/inet.h>
#    include <netinet/in.h>
#  endif // BSD
#  include <cerrno>
#  include <netdb.h>
#  include <unistd.h>
#endif


// init the object
MESSAGE_QUEUE::MESSAGE_QUEUE() {
	first_message = last_message = nullptr;
}

// do clean up on all messages
MESSAGE_QUEUE::~MESSAGE_QUEUE() {
	Erase_All();
}

// add a new message to the queue
// Returns true on success and false is an error occures
bool MESSAGE_QUEUE::Add( char *some_text, int to ) {
	sMessage *new_message;

	if ( !some_text ) {
		return false;
	}

	new_message = (sMessage *)calloc( 1, sizeof( sMessage ) );
	if ( !new_message ) {
		return false;
	}

	// fill the message structure
	new_message->text = strdup( some_text );
	new_message->to   = to;
	// next is already cleared by calloc

	// empty line, create new line
	if ( !first_message ) {
		first_message = new_message;
		last_message  = new_message;
	}

	else // we have a line, add it to the end
	{
		last_message->next = new_message;
		last_message       = new_message;
	}
	return true;
}

// retreive a message and erase it from the queue
// returns a message on success and nullptr on failure
sMessage *MESSAGE_QUEUE::Read() {
	sMessage *my_message;

	my_message = Peek(); // grab next message
	if ( my_message ) {
		Erase(); // clear it from the queue
	}

	return my_message;
}

// returns a message from the queue without removing it from the line
// Returns a message on success or a nullptr on failure
sMessage *MESSAGE_QUEUE::Peek() const {
	sMessage *my_message;

	// see if there is a message to get
	if ( ( !first_message ) || ( !first_message->text ) ) {
		return nullptr;
	}

	// we want to make a copy of the message, not just pass back a pointer
	my_message = (sMessage *)calloc( 1, sizeof( sMessage ) );
	if ( !my_message ) {
		return nullptr;
	}
	size_t text_len  = strlen( first_message->text );
	my_message->text = (char *)calloc( text_len + 1, sizeof( char ) );
	if ( !my_message->text ) {
		free( my_message );
		return nullptr;
	}

	// we have an empty message. Now fill it
	strncpy( my_message->text, first_message->text, text_len );
	my_message->to = first_message->to;

	return my_message;
}

// Finds the next message destined "to" a given player
// This function returns the first message it finds and
// erases it. The message is returned on success or a
// nullptr is returned if no message is found.
sMessage *MESSAGE_QUEUE::Read_To( int my_to ) {
	sMessage *current, *previous = nullptr;
	bool     found = false;

	// search for matching to field
	current = first_message;
	while ( ( current ) && ( !found ) ) {
		if ( current->to == my_to ) {
			found = true;
		} else {
			previous = current;
			current  = (sMessage *)current->next;
		}
	}

	if ( !current ) {
		return nullptr;
	}

	// found match, create a copy and erase the original
	auto *my_message = (sMessage *)calloc( 1, sizeof( sMessage ) );
	if ( !my_message ) {
		return nullptr;
	}
	size_t text_len  = strlen( current->text );
	my_message->text = (char *)calloc( text_len + 1, sizeof( char ) );
	if ( !my_message->text ) {
		free( my_message );
		return nullptr;
	}

	my_message->to = current->to;
	strncpy( my_message->text, current->text, text_len );
	if ( previous ) {
		previous->next = current->next;
	} else {
		first_message = (sMessage *)current->next;
	}

	if ( last_message == current ) {
		last_message = previous;
	}

	free( current->text );
	free( current );
	return my_message;
}

// Erases the next message in the line without returning anything
void MESSAGE_QUEUE::Erase() {
	sMessage *next_in_line;

	if ( first_message ) {
		next_in_line = (sMessage *)first_message->next;
		// clean up
		if ( first_message->text ) {
			free( first_message->text );
			first_message->text = nullptr;
		}
		free( first_message );

		// see if we are at the end of the list
		if ( last_message == first_message ) {
			last_message = next_in_line;
		}

		first_message = next_in_line;
	}
}

// This function erases all messages in the queue.
void MESSAGE_QUEUE::Erase_All() {
	sMessage *current, *coming_up;

	current = first_message;
	while ( current ) {
		coming_up = (sMessage *)current->next;
		if ( current->text ) {
			free( current->text );
		}
		free( current );
		current = coming_up;
	}

	first_message = last_message = nullptr;
}

//
// Past this point we have socket functions.
//
//
#ifdef NETWORK


// Create a socket for listening. Returns a listening socket
// on success or -1 on failure.
int Setup_Server_Socket( int port ) {
	int                listensocket;

	struct sockaddr_in myaddr {};

	listensocket           = socket( AF_INET, SOCK_STREAM, 0 );
	myaddr.sin_port        = htons( port );
	myaddr.sin_addr.s_addr = INADDR_ANY;
	if ( bind( listensocket, (struct sockaddr *)&myaddr, sizeof( myaddr ) ) < 0 ) {
		printf( "Bind failed: %s\n", strerror( errno ) );
		return -1;
	}
	if ( listen( listensocket, 5 ) ) {
		printf( "Listen failed: %s\n", strerror( errno ) );
		return -1;
	}

	return listensocket;
}

// Connect to a remote server. Returns -1 on failure or
// a socket (int) on success.
int Setup_Client_Socket( char *server_name, char const *port ) {
	int                socket_num, port_number = 4711;

	struct sockaddr_in server_address {};
	struct hostent    *server;

	SAFE_STOI( port_number, port );
	socket_num = socket( AF_INET, SOCK_STREAM, 0 );
	if ( socket_num < 0 ) {
		return -1;
	}
	server = gethostbyname( server_name );
	if ( !server ) {
		return -1;
	}
	bzero( (char *)&server_address, sizeof( server_address ) );
	server_address.sin_family = AF_INET;
	bcopy( (char *)server->h_addr, (char *)&server_address.sin_addr.s_addr, server->h_length );
	server_address.sin_port = htons( port_number );

	// try to connect
	if ( connect( socket_num, (sockaddr *)&server_address, sizeof( server_address ) ) < 0 ) {
		return -1;
	}

	return socket_num;
}

// Accepts an incoming connection request. Returns the
// new socket connection on success or -1 on failure.
int Accept_Incoming_Connection( int my_socket ) {
	int             new_connection;
	socklen_t       my_length;

	struct sockaddr new_address {};

	my_length      = sizeof( new_address );
	new_connection = accept( my_socket, (struct sockaddr *)&new_address, &my_length );
	return new_connection;
}

// Take whatever is in the message and send it
// to a socket.
// Returns a negative number on failure. Zero and
// positive numbers indicate success.
int Send_Message( sMessage *mess, int to_socket ) {
	char buffer[ MAX_MESSAGE_LENGTH ];

	strncpy( buffer, mess->text, MAX_MESSAGE_LENGTH );
	buffer[ MAX_MESSAGE_LENGTH - 1 ] = '\0';
	return static_cast< int >( write( to_socket, buffer, strlen( buffer ) ) );
}

// Read data from a socket and put it in a message
// Returns the message on success and nullptr on failure.
// Note: the "to" field of the message is not set.
sMessage *Receive_Message( int from_socket ) {
	sMessage *my_message;
	char     buffer[ MAX_MESSAGE_LENGTH ];

	my_message = (sMessage *)calloc( 1, sizeof( sMessage ) );
	if ( !my_message ) {
		return nullptr;
	}

	memset( buffer, '\0', MAX_MESSAGE_LENGTH );
	ssize_t bytes_read = read( from_socket, buffer, MAX_MESSAGE_LENGTH );
	if ( bytes_read > 0 ) {
		buffer[ MAX_MESSAGE_LENGTH - 1 ] = '\0';
		my_message->text                 = (char *)calloc( strlen( buffer ), sizeof( char ) );
		if ( !my_message->text ) {
			free( my_message );
			return nullptr;
		}
		strcpy( my_message->text, buffer );
		return my_message;
	} else // error while reading
	{
		free( my_message );
		return nullptr;
	}
}

void Clean_Up_Server_Socket( int my_socket ) {
	close( my_socket );
}

void Clean_Up_Client_Socket( int my_socket ) {
	close( my_socket );
}

// This function checks the socket to see if there is data ready to
// be read. If there is data ready, then the function returns true. If
// an error occures, the function returns -1. If there is no data
// and no error, the function returns false.
int Check_For_Incoming_Data( int socket_number ) {
	fd_set         rfds;

	struct timeval tv {};

	int            retval;

	FD_ZERO( &rfds );
	FD_SET( socket_number, &rfds );

	tv.tv_sec  = 0;
	tv.tv_usec = 0;

	retval     = select( socket_number + 1, &rfds, nullptr, nullptr, &tv );

	if ( retval == -1 ) {
		return -1;
	}

	return retval ? 1 : 0;
}

// This function checks the passed socket for errors. If the socket
// is error-free, the function returns false. If an exception
// has occured, the function returns true.
bool Check_For_Errors( int socket_number ) {
	fd_set         exds;

	struct timeval tv {};

	int            expval;

	FD_ZERO( &exds );
	FD_SET( socket_number, &exds );
	tv.tv_sec  = 0;
	tv.tv_usec = 0;

	expval     = select( socket_number + 1, nullptr, nullptr, &exds, &tv );

	if ( expval == -1 ) {
		return true;
	}

	return expval == 0;
}

// This function will probably be called as a separate thread at the
// start of the game. It will set up a listening port, accept
// incoming connections and manage them. That is, they will be
// passed on to AI players.
void Send_And_Receive( void *all_the_data ) {
	auto *send_receive_data = (sSendReceive *)all_the_data;
	int   server_socket, new_socket;
	int   status, counter;

	bool  found;

	// set up listening socket
	server_socket = Setup_Server_Socket( send_receive_data->listening_port );
	if ( server_socket == -1 ) {
		printf( "Error creating listening socket.\n" );
		return;
	}

	while ( !send_receive_data->shut_down ) {
		// check for incoming connections
		status = Check_For_Incoming_Data( server_socket );
		if ( status ) {
			new_socket = Accept_Incoming_Connection( server_socket );
			printf( "Accepted connection.\n" );
			// give connection to AI player
			found   = false;
			counter = 0;
			while ( ( !found ) && ( counter < env.numGamePlayers ) ) {
				if ( ( env.players[ counter ]->type >= USELESS_PLAYER )
				     && ( env.players[ counter ]->type <= DEADLY_PLAYER ) ) {
					found                                 = true;
					env.players[ counter ]->server_socket = new_socket;
					env.players[ counter ]->previous_type = env.players[ counter ]->type;
					env.players[ counter ]->type          = NETWORK_CLIENT;
					printf( "Assigned connection to %s\n", env.players[ counter ]->getName() );
				} else {
					counter++;
				}
			}
			// in case we did not find a match
			if ( !found ) {
				printf( "Unable to assign new connection to player.\n" );
				SAFE_WRITE( new_socket, "%s", "NOROOM" );
				close( new_socket );
			}
		}

		// consider resting for a moment?
		LINUX_SLEEP;
	}

	// clean up everything
	printf( "Cleaning up networking\n" );
	Clean_Up_Server_Socket( server_socket );
	counter = 0;
	while ( counter < env.numGamePlayers ) {
		if ( env.players[ counter ]->type == NETWORK_CLIENT ) {
			SAFE_WRITE( env.players[ counter ]->server_socket, "%s", "CLOSE" );
			close( env.players[ counter ]->server_socket );
		}
		counter++;
	}
}


#endif
