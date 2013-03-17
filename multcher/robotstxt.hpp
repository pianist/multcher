#ifndef __MULTCHER_ROBOTS_TXT_INFORMER__
#define __MULTCHER_ROBOTS_TXT_INFORMER__

#include <string>
#include <vector>
#include <map>
#include "consumer.hpp"

namespace multcher
{

struct domain_robotstxt_t
{
	struct item_t
	{
		bool allow;
		std::string condition;
	};
	std::vector<item_t> items;
	std::string crawl_delay;

	time_t last_update;
	bool loaded;
	bool broken;

	domain_robotstxt_t()
	  : last_update(0)
	  , loaded(false)
	  , broken(false)
	{
	}

	void parse_from_source(const char* src, const char* bot_id);

	// 0 disallow, 1 allow
	bool check_uri(const std::string& uri) const;
};

struct robotstxt_check_result_t
{
	bool allow;
	bool disallow;
	bool unknown;
	bool update_robots_txt;
	std::string domain;
};

struct robotstxt_t
{
	typedef std::map<std::string, domain_robotstxt_t> data_type;
	data_type _data;

	void check_url(const std::string& url, robotstxt_check_result_t& r);
};

class robotstxt_consumer_t : public consumer_t
{
	std::string _myself_robot_id;

	robotstxt_t rtxt_data;
	pthread_mutex_t rtxt_data_mutex;

public:
	robotstxt_consumer_t(const std::string& myself_robot_id);
	~robotstxt_consumer_t() throw();

	void check_url(const std::string& url, robotstxt_check_result_t& r);

	void completely_failed(const request_t& req);
	void robotstxt_disallowed(const request_t& req);
	void receive(const request_t& req, const response_t& resp, CURLcode code);
};





} // namespace multcher

#endif // __MULTCHER_ROBOTS_TXT_INFORMER__

