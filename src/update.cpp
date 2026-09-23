#include "update.h"

#include "externs.h"

#include <cstdio>
#include <string>

#ifdef ATANKS_HAVE_CURL
#  include <curl/curl.h>
#endif

namespace {

/// @brief GitHub releases endpoint (TODO-PF-2). Tag comparison needs no other infrastructure.
char const* const ATANKS_RELEASES_URL = "https://api.github.com/repos/EdenWorX/atanks/releases/latest";

#ifdef ATANKS_HAVE_CURL
/// @brief libcurl write callback appending response chunks.
/// @return the consumed byte count.
size_t append_response( char* data, size_t size, size_t count, void* target ) {
	auto* body = static_cast< std::string* >( target );
	body->append( data, size * count );
	return size * count;
}

/// @brief Fetch the latest release tag over HTTPS; silent on failure (offline, no network).
/// @param[out] tag Tag value, e.g. "v6.7.1".
/// @return true when a tag was fetched.
bool fetch_latest_tag( std::string& tag ) {
	CURL* handle = curl_easy_init();
	if ( !handle ) {
		return false;
	}
	std::string body;
	curl_easy_setopt( handle, CURLOPT_URL, ATANKS_RELEASES_URL );
	curl_easy_setopt( handle, CURLOPT_USERAGENT, "atanks/" VERSION );
	curl_easy_setopt( handle, CURLOPT_TIMEOUT, 10L );
	curl_easy_setopt( handle, CURLOPT_CONNECTTIMEOUT, 5L );
	curl_easy_setopt( handle, CURLOPT_WRITEFUNCTION, append_response );
	curl_easy_setopt( handle, CURLOPT_WRITEDATA, &body );
	CURLcode const result = curl_easy_perform( handle );
	long status           = 0;
	curl_easy_getinfo( handle, CURLINFO_RESPONSE_CODE, &status );
	curl_easy_cleanup( handle );
	if ( ( CURLE_OK != result ) || ( 200 != status ) ) {
		return false;
	}
	return extract_tag_name( body, tag );
}
#endif // ATANKS_HAVE_CURL

} // namespace

void UpdateData::operator() () {
#ifdef ATANKS_HAVE_CURL
	if ( !env.check_for_updates ) {
		return;
	}
	std::string tag;
	if ( !fetch_latest_tag( tag ) ) {
		return;
	}
	VersionTuple remote{};
	VersionTuple local{};
	if ( !parse_version_tuple( tag.c_str(), remote ) || !parse_version_tuple( VERSION, local ) ) {
		return;
	}
	if ( version_is_newer( remote, local ) ) {
		snprintf( update_string,
		          sizeof( update_string ),
		          "A new version, %d.%d.%d, is ready for download.",
		          remote.major,
		          remote.minor,
		          remote.patch );
	}
#endif // ATANKS_HAVE_CURL
}
