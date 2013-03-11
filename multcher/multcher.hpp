#ifndef __MULTCHER_HPP_DFGSDFGSDFGSDFJHGFDS__
#define __MULTCHER_HPP_DFGSDFGSDFGSDFJHGFDS__

#include <stdarg.h>
#include <stdexcept>
#include <deque>
#include <map>
#include <vector>
#include <pthread.h>
#include <coda/synque.hpp>
#include <curl/curl.h>

struct multcher_request
{
	std::string url;
	std::string accept_language;
	unsigned max_redirect;
	bool fail_on_error;

	multcher_request()
	  : max_redirect(5)
	  , fail_on_error(false)
	{
	}
};

struct multcher_redirect
{
	int code;
	std::string location;
	multcher_redirect()
	  : code(0)
	{
	}
};

typedef std::vector<multcher_redirect> multcher_redirects;

struct multcher_response
{
	int code;
	std::string content_type;
	std::string server;
	std::string body;
	multcher_redirects redirects;

	multcher_response()
	  : code(0)
	{
	}
};

struct multcher_internal
{
	multcher_request req;
	multcher_response resp;
	curl_slist* additional_headers;

	multcher_internal()
	  : additional_headers(0)
	{
	}
};

struct multcher_consumer
{
	virtual ~multcher_consumer() throw()
	{
	}

	virtual void receive(const multcher_request& req, const multcher_response& resp, CURLcode code) = 0;
};

class multcher
{
	coda::synque<multcher_request> queue;

	typedef std::map<CURL*, multcher_internal> internals_t;
	internals_t internal_data;

	pthread_t th_fetcher;
	bool shutdown_asap;
	bool thread_started;
	int max_concur;
	int queries_running;
	CURLM* cmh;
	multcher_consumer* consumer;

	void add_requests(bool can_lock);
public:
	multcher();
	~multcher();
	void set_concurrent_max_count(int v) { max_concur = v; }
	void set_consumer(multcher_consumer* c) { consumer = c; }

	void configure();
	void create_working_thread();
	void join_working_thread();


	void add_request(const multcher_request& req);


	void working_thread_proc();
};





#endif // __MULTCHER_HPP_DFGSDFGSDFGSDFJHGFDS__

