#ifndef ATANKS_SRC_UPDATE_H_INCLUDED
#define ATANKS_SRC_UPDATE_H_INCLUDED 1


#define UPDATE_STR_LENGTH 256

// rewritten struct to be used with C++11 threads.
struct update_data {
	char* server_name = nullptr;
	char* host_name   = nullptr;
	char* remote_file = nullptr;
	char  update_string[ 1024 ]{ 0x0 };

	explicit update_data( char const* server_, char const* remote_, char const* host_ );
	~update_data();

	void operator() ();
};

#endif // ATANKS_SRC_UPDATE_H_INCLUDED
