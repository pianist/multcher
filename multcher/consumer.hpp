#ifndef __MULTCHER_CONSUMER_HPP_QWE321HGFDS__
#define __MULTCHER_CONSUMER_HPP_QWE321HGFDS__

#include <map>
#include <vector>
#include <string>
#include <curl/curl.h>

namespace multcher
{

struct request_t
{
	std::string url;
	std::string accept_language;
	unsigned max_redirect;
	bool fail_on_error;

	// internal usage only!!!
	bool is_internal;
	std::string domain;

	request_t()
	  : max_redirect(5)
	  , fail_on_error(false)
	  , is_internal(false)
	{
	}
};

struct redirect_t
{
	int code;
	std::string location;
	redirect_t()
	  : code(0)
	{
	}
};

typedef std::vector<redirect_t> redirects_t;

struct response_t
{
	int code;
	std::string content_type;
	std::string server;
	std::string body;
	redirects_t redirects;

	response_t()
	  : code(0)
	{
	}
};

struct consumer_t
{
	virtual ~consumer_t() throw()
	{
	}

	virtual void completely_failed(const request_t& req) = 0;

	virtual void receive(const request_t& req, const response_t& resp, CURLcode code) = 0;
};

} // namespace multcher

#endif // __MULTCHER_CONSUMER_HPP_QWE321HGFDS__

