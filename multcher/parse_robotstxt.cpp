#include "robotstxt.hpp"
#include <string.h>

static const char* get_line(const char* s, const char** l_b, const char** l_e)
{
	while (*s && strchr("\t\n\r ", *s)) ++s;

	if (!*s) return 0;

	*l_b = s;
	while (*s && (*s != '\r') && (*s != '\n') && (*s != '#')) ++s;
	*l_e = s;

	if (*s == '#')
	{
		while (*s && (*s != '\r') && (*s != '\n')) ++s;
	}

	while ((*s == '\r') || (*s == '\n')) ++s;

	return s;
}

inline static bool command_name_cmp(const char* name, const char* s, const char** param)
{
	size_t sz = strlen(name);
	if (strncasecmp(name, s, sz) != 0) return false;
	
	const char* param_b = s + sz;
	while (*param_b && ((*param_b == ' ') || (*param_b == '\t'))) ++param_b;

	*param = param_b;
	return true;
}

static int command_id(const char* b, const char* e, const char** param)
{
	if (command_name_cmp("User-agent:", b, param)) return 1;
	if (command_name_cmp("Disallow:", b, param)) return 2;
	if (command_name_cmp("Allow:", b, param)) return 3;
	if (command_name_cmp("Sitemap:", b, param)) return 4;
	if (command_name_cmp("Crawl-delay:", b, param)) return 5;

	return 0;
}



void multcher::domain_robotstxt_t::parse_from_source(const char* src, const char* bot_id)
{
	const char* b;
	const char* e;

	size_t bot_id_len = strlen(bot_id);
	bool accept = true;

	while ((src = get_line(src, &b, &e)))
	{
		if (e == b) continue;

		const char* param;
		int cmd = command_id(b, e, &param);
		switch (cmd)
		{
			case 1:
				accept = (*param == '*') || (strncasecmp(bot_id, param, bot_id_len) == 0);
				break;

			case 2:
			case 3:
				if (accept)
				{
					domain_robotstxt_t::item_t item;
					item.allow = cmd == 3;
					item.condition.assign(param, e - param);
					items.push_back(item);
				}
				break;
			case 5:
				crawl_delay.assign(param, e - param);
		}
	}
}

