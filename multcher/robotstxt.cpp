#include "robotstxt.hpp"
#include <string.h>

multcher::robotstxt_consumer_t::robotstxt_consumer_t(const std::string& myself_robot_id)
  : _myself_robot_id(myself_robot_id)
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

	if (it->second.broken)
	{
		// XXX we try to download robots.txt not more than one time per 5 min
		if (::time(0) - it->second.last_update > 5 * 60)
		{
			r.update_robots_txt = true;
		}
		else
		{
			r.update_robots_txt = false;
		}

		// we can not download anything until we load robots.txt
		if (!it->second.loaded)
		{
			r.allow = false;
			r.unknown = false;
			return;
		}
	}

	if (!it->second.loaded)
	{
		r.allow = false;
		r.unknown = true;
		r.update_robots_txt = false;
		return;
	}

	r.allow = it->second.check_uri(uri);
	r.update_robots_txt = ::time(0) - it->second.last_update > 3 * 24 * 60 * 60;
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

void multcher::robotstxt_consumer_t::completely_failed(const request_t& req)
{
	pthread_mutex_lock(&rtxt_data_mutex);

	domain_robotstxt_t& dr = rtxt_data._data[req.domain];
	dr.last_update = ::time(0);
	dr.broken = true;

	pthread_mutex_unlock(&rtxt_data_mutex);
}

void multcher::robotstxt_consumer_t::receive(const request_t& req, const response_t& resp, CURLcode code)
{
	pthread_mutex_lock(&rtxt_data_mutex);
	domain_robotstxt_t dr = rtxt_data._data[req.domain];
	pthread_mutex_unlock(&rtxt_data_mutex);
	dr.last_update = ::time(0);

	switch (code)
	{
		case CURLE_COULDNT_RESOLVE_HOST:
		case CURLE_COULDNT_CONNECT:
		case CURLE_OPERATION_TIMEDOUT:
			// TODO add additional "later robots.txt queue"
			dr.broken = true;
			break;

		case CURLE_OK:
			dr.loaded = true;
			dr.broken = false;
			dr.parse_from_source(resp.body.c_str(), _myself_robot_id.c_str());
			break;

		default:
			dr.loaded = true;
			dr.broken = false;
			break;
	}

	pthread_mutex_lock(&rtxt_data_mutex);
	rtxt_data._data[req.domain] = dr;
	pthread_mutex_unlock(&rtxt_data_mutex);
}

