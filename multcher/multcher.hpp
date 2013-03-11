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
#include "consumer.hpp"

namespace multcher
{


struct multcher_internal_t
{
	request_t req;
	response_t resp;
	curl_slist* additional_headers;

	multcher_internal_t()
	  : additional_headers(0)
	{
	}
};


class downloader
{
	coda::synque<request_t> queue;

	class multcher_internal;
	std::map<CURL*, multcher_internal_t> internal_data;

	pthread_t th_fetcher;
	bool shutdown_asap;
	bool thread_started;
	int max_concur;
	int queries_running;
	CURLM* cmh;
	consumer_t* consumer;

	void add_requests(bool can_lock);
public:
	downloader();
	~downloader();
	void set_concurrent_max_count(int v) { max_concur = v; }
	void set_consumer(consumer_t* c) { consumer = c; }

	void configure();
	void create_working_thread();
	void join_working_thread();


	void add_request(const request_t& req);


	void working_thread_proc();
};



} // namespace multcher

#endif // __MULTCHER_HPP_DFGSDFGSDFGSDFJHGFDS__

