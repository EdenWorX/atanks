#ifndef ATANKS_SRC_UPDATE_H_INCLUDED
#define ATANKS_SRC_UPDATE_H_INCLUDED 1

// rewritten struct to be used with C++11 threads.
/** @struct update_data
 * @brief Legacy update checker request data.
 **/
struct update_data {
	char* server_name = nullptr;        ///< Update server name.
	char* host_name   = nullptr;        ///< Update host name.
	char* remote_file = nullptr;        ///< Remote version file.
	char  update_string[ 1024 ]{ 0x0 }; ///< Fetched update message.

	/// Construct update request data.
	explicit update_data( char const* server_, char const* remote_, char const* host_ );
	/// Destroy update request data.
	~update_data();

	/// Fetch the update message.
	void operator() ();
};

#endif // ATANKS_SRC_UPDATE_H_INCLUDED
