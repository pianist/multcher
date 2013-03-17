#include "multcher.hpp"

multcher::scheduler::scheduler()
{
	pthread_mutex_init(&di_mutex, 0);
}

multcher::scheduler::~scheduler()
{
	pthread_mutex_destroy(&di_mutex);
}

unsigned multcher::scheduler::wait_start_download(const std::string& domain)
{
	unsigned ret = 0;

	pthread_mutex_lock(&di_mutex);
	domain_info_t& di = _di[domain];
	time_t ntm = time(0);

	if (di.now_fetching >= di.max_fetching)
	{
		ret += di.crawl_delay;
	}
	else
	{
		if (di.last_finish + di.crawl_delay > ntm)
		{
			ret += di.last_finish + di.crawl_delay - ntm;
		}
		else if (di.last_start + di.crawl_delay > ntm)
		{
			ret += di.last_start + di.crawl_delay - ntm;
		}
	}

	if (!ret)
	{
		di.last_start = ntm;
		di.now_fetching++;
	}

	pthread_mutex_unlock(&di_mutex);
	return ret;
}

void multcher::scheduler::set_finished_ok(const std::string& domain)
{
	pthread_mutex_lock(&di_mutex);
	domain_info_t& di = _di[domain];
	di.now_fetching--;
	di.last_finish = time(0);
	pthread_mutex_unlock(&di_mutex);
}

