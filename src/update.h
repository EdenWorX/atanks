#ifndef ATANKS_UPDATE_H_INCLUDED
#define ATANKS_UPDATE_H_INCLUDED 1

#include <cstdint>
#include <cstdlib>
#include <string>

/** @struct VersionTuple
 * @brief Numeric MAJOR.MINOR.PATCH triple for update comparison.
 **/
struct VersionTuple {
	int32_t major = 0; ///< Major version.
	int32_t minor = 0; ///< Minor version.
	int32_t patch = 0; ///< Patch version.
};

/// @brief Parse "vMAJOR.MINOR.PATCH" (leading non-digits skipped, missing parts stay zero).
/// @param[in] text Text holding the version, e.g. "v6.7.1".
/// @param[out] out Parsed triple.
/// @return true when at least the major version parsed.
inline bool parse_version_tuple( char const* text, VersionTuple& out ) {
	out = VersionTuple{};
	if ( !text ) {
		return false;
	}
	// Skip any leading non-digits (e.g. the 'v' in "v6.7.1").
	while ( ( *text < '0' ) || ( *text > '9' ) ) {
		if ( !*text ) {
			return false;
		}
		++text;
	}
	int32_t     parts[ 3 ]{ 0, 0, 0 };
	int32_t     count  = 0;
	char const* cursor = text;
	while ( ( count < 3 ) && *cursor ) {
		char*      end   = nullptr;
		long const value = std::strtol( cursor, &end, 10 );
		if ( ( end == cursor ) || ( value < 0 ) || ( value > INT32_MAX ) ) {
			break;
		}
		parts[ count++ ] = static_cast< int32_t >( value );
		cursor           = end;
		if ( '.' == *cursor ) {
			++cursor;
		} else {
			break;
		}
	}
	if ( 0 == count ) {
		return false;
	}
	out.major = parts[ 0 ];
	out.minor = parts[ 1 ];
	out.patch = parts[ 2 ];
	return true;
}

/// @brief True when the remote triple is strictly newer (numeric per-part comparison).
/// @param[in] remote Version reported by the endpoint.
/// @param[in] local Version this binary was built as.
/// @return true when remote is newer.
inline bool version_is_newer( VersionTuple const& remote, VersionTuple const& local ) {
	if ( remote.major != local.major ) {
		return remote.major > local.major;
	}
	if ( remote.minor != local.minor ) {
		return remote.minor > local.minor;
	}
	return remote.patch > local.patch;
}

/// @brief Extract the tag_name string from a GitHub releases/latest JSON body.
/// @param[in] json Response body.
/// @param[out] tag Tag value without quotes, e.g. "v6.7.1".
/// @return true when a non-empty tag value was found.
inline bool extract_tag_name( std::string const& json, std::string& tag ) {
	tag.clear();
	std::string::size_type const key = json.find( "\"tag_name\"" );
	if ( std::string::npos == key ) {
		return false;
	}
	std::string::size_type const colon = json.find( ':', key );
	std::string::size_type const open =
		( std::string::npos == colon ) ? std::string::npos : json.find( '"', colon );
	if ( std::string::npos == open ) {
		return false;
	}
	std::string::size_type const close = json.find( '"', open + 1 );
	if ( std::string::npos == close ) {
		return false;
	}
	tag = json.substr( open + 1, close - open - 1 );
	return !tag.empty();
}

// rewritten struct to be used with C++11 threads.
/** @struct UpdateData
 * @brief GitHub release update checker request data.
 **/
struct UpdateData {
	char update_string[ 1024 ]{ 0x0 }; ///< Fetched update message.

	/// Fetch the update message.
	void operator() ();
};

#endif // ATANKS_UPDATE_H_INCLUDED
