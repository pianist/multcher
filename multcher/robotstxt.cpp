#include "robotstxt.hpp"
#include <string.h>

multcher::robotstxt_consumer_t::robotstxt_consumer_t()
{
	pthread_mutex_init(&rtxt_data_mutex, 0);
}

multcher::robotstxt_consumer_t::~robotstxt_consumer_t() throw()
{
	pthread_mutex_destroy(&rtxt_data_mutex);
}



static void get_url_domain_and_uri(const std::string& url, std::string& domain, std::string& uri)
{
	const char* s = url.c_str();

	if (strncmp(s, "http://", 7) == 0)
	{
		s += 7;
	}
	else if (strncmp(s, "https://", 8) == 0)
	{
		s += 8;
	}
	else
	{
		domain.clear();
		uri.clear();
		return;
	}

	const char* t = s;
	while (*t)
	{
		if (*t == ':') break;
		if (*t == '/') break;
		++t;
	}

	domain.assign(s, t - s);

	while (*t && (*t != '/')) ++t;

	uri = *t ? t : "/";
}

void multcher::robotstxt_t::check_url(const std::string& url, robotstxt_check_result_t& r)
{
	std::string uri;
	get_url_domain_and_uri(url, r.domain, uri);

	if (r.domain.empty())
	{
		r.update_robots_txt = false;
		r.unknown = false;
		r.allow = false;
		return;
	}

	data_type::const_iterator it = _data.find(r.domain);
	if (it == _data.end())
	{
		_data[r.domain].loaded = false;
		r.allow = false;
		r.unknown = true;
		r.update_robots_txt = true;
		return;
	}

	if (it->second.loaded == false)
	{
		r.allow = false;
		r.unknown = true;
		r.update_robots_txt = false;
		return;
	}

	r.allow = it->second.check_uri(uri);
	// TODO check whether update robots.txt ...
	// and set r.update_robots_txt...
	r.update_robots_txt = false;
	r.unknown = false;
}

bool multcher::domain_robotstxt_t::check_uri(const std::string& uri) const
{
	return true;
}

void multcher::robotstxt_consumer_t::check_url(const std::string& url, robotstxt_check_result_t& r)
{
	pthread_mutex_lock(&rtxt_data_mutex);
	rtxt_data.check_url(url, r);
	pthread_mutex_unlock(&rtxt_data_mutex);
}

void multcher::robotstxt_consumer_t::receive(const request_t& req, const response_t& resp, CURLcode code)
{
	domain_robotstxt_t dr;
	dr.last_update = ::time(0);
	dr.loaded = true;
	// TODO

	pthread_mutex_lock(&rtxt_data_mutex);
	rtxt_data._data[req.domain] = dr;
	pthread_mutex_unlock(&rtxt_data_mutex);
}

