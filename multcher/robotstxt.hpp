#ifndef __MULTCHER_ROBOTS_TXT_INFORMER__
#define __MULTCHER_ROBOTS_TXT_INFORMER__

namespace multcher
{

struct domain_robotstxt_t
{
	std::vector<std::string> allow;
	std::vector<std::string> disallow;

	void parse_from_source(const char* src, const char* bot_id);

	// 0 disallow, 1 allow
	bool check_uri(const char* uri) const;
};


class robotstxt_t
{
	typedef std::map<std::string, domain_robotstxt_t> data_type;
	data_type _data;

public:
	// -1 -- should read robots.txt, 0 disallow, 1 allow
	int check_url(const char* url) const;
	
};





}

#endif // __MULTCHER_ROBOTS_TXT_INFORMER__

