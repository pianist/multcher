#include "multcher.hpp"
#include <string.h>
#include <stdlib.h>

multcher::downloader::downloader(const std::string& myself_robot_id)
  : shutdown_asap(false)
  , thread_started(false)
  , fetcher_wont_send_more(false)
  , max_concur(10)
  , consumer(0)
  , rtxt_consumer(myself_robot_id)
{
	cmh = curl_multi_init();
	pthread_mutex_init(&unknown_urls_mutex, 0);
}

multcher::downloader::~downloader()
{
	curl_multi_cleanup(cmh);
	pthread_mutex_destroy(&unknown_urls_mutex);
}

void multcher::downloader::add_request(const request_t& req)
{
	// check whether we know robots.txt of domain
	robotstxt_check_result_t cr;

	rtxt_consumer.check_url(req.url, cr);

	if (cr.update_robots_txt)
	{
		request_t r;

		char buf[1024];
		snprintf(buf, 1024, "http://%s/robots.txt", cr.domain.c_str());
		r.url = buf;
		r.is_internal = true;
		r.domain = cr.domain;
		queue_scheduler.add(r);
	}

	if (cr.allow)
	{
		request_t r = req;
		r.is_internal = false;
		r.domain = cr.domain;
		queue_scheduler.add(r);
	}

	if (cr.unknown)
	{
		pthread_mutex_lock(&unknown_urls_mutex);
		unknown_urls[cr.domain].push_back(req);
		pthread_mutex_unlock(&unknown_urls_mutex);
	}

	if (!cr.update_robots_txt && !cr.allow && !cr.unknown)
	{
		if (cr.disallow)
		{
			consumer->robotstxt_disallowed(req);
		}
		else
		{
			consumer->completely_failed(req);
		}
	}
}

static size_t cb_response_data(char* ptr, size_t size, size_t nmemb, void* userdata)
{
	multcher::response_t* resp = (multcher::response_t*)userdata;
	resp->body.append(ptr, size * nmemb);

	return nmemb;
}

static size_t correct_len(const char* s, size_t len)
{
	for (size_t i = 0; i < len; ++i)
	{
		if (s[i] == '\r') return i;
		if (s[i] == '\n') return i;
	}

	return len;
}

static std::string check_header(char* ptr, size_t len, const char* s)
{
	std::string r;
	size_t s_len = strlen(s);

	if (strncasecmp(ptr, s, s_len) == 0)
	{
		len = correct_len(ptr, len);

		r.assign(ptr + s_len, len - s_len);
	}

	return r;
}

static size_t cb_response_header(void* ptr, size_t size, size_t nmemb, void* userdata)
{
	multcher::response_t* resp = (multcher::response_t*)userdata;
	std::string h;

	if (strncasecmp((char*)ptr, "HTTP/", 5) == 0)
	{
		resp->code = atoi((char*)ptr + 9);
		return nmemb;
	}

	h = check_header((char*)ptr, size * nmemb, "Location: ");
	if (!h.empty())
	{
		multcher::redirect_t redir;
		redir.location = h;
		redir.code = resp->code;

		resp->redirects.push_back(redir);
	}

	h = check_header((char*)ptr, size * nmemb, "Content-Type: ");
	if (!h.empty())
	{
		resp->content_type = h;
	}

	h = check_header((char*)ptr, size * nmemb, "Server: ");
	if (!h.empty())
	{
		resp->server = h;
	}
	

	return nmemb;
}

void multcher::downloader::add_requests(bool can_lock)
{
	request_t r;

	while (queue_fetcher.get(r, can_lock) >= 0)
	{
		can_lock = false;

		CURL* ch = curl_easy_init();
		multcher::multcher_internal_t& p = internal_data[ch];
		p.req = r;

		curl_easy_setopt(ch, CURLOPT_URL, r.url.c_str());
		curl_easy_setopt(ch, CURLOPT_FOLLOWLOCATION, 1);
		curl_easy_setopt(ch, CURLOPT_MAXREDIRS, 3);
		curl_easy_setopt(ch, CURLOPT_WRITEFUNCTION, cb_response_data);
		curl_easy_setopt(ch, CURLOPT_WRITEDATA, &p.resp);
		curl_easy_setopt(ch, CURLOPT_HEADERFUNCTION, cb_response_header);
		curl_easy_setopt(ch, CURLOPT_HEADERDATA, &p.resp);
		curl_easy_setopt(ch, CURLOPT_ENCODING, "gzip,deflate");
		curl_easy_setopt(ch, CURLOPT_TIMEOUT, 10);

		if (!r.fail_on_error) curl_easy_setopt(ch, CURLOPT_FAILONERROR, 1);

		if (!r.accept_language.empty())
		{
			char buf[1024];
			snprintf(buf, 1024, "Accept-Language: %s", r.accept_language.c_str());
			p.additional_headers = curl_slist_append(p.additional_headers, buf);
		}

		if (p.additional_headers)
		{
			curl_easy_setopt(ch, CURLOPT_HTTPHEADER, p.additional_headers);
		}

		curl_multi_add_handle(cmh, ch);

		queries_running++;
		if (queries_running >= max_concur) return;
	}
}

static void* job_working_thread_proc_fetcher(void* p)
{
	multcher::downloader* m = (multcher::downloader*)p;
	m->working_thread_proc_fetcher();
	return 0;
}

static void* job_working_thread_proc_scheduler(void* p)
{
	multcher::downloader* m = (multcher::downloader*)p;
	m->working_thread_proc_scheduler();
	return 0;
}

void multcher::downloader::create_working_thread()
{
	pthread_create(&th_fetcher, 0, job_working_thread_proc_fetcher, this);
	pthread_create(&th_scheduler, 0, job_working_thread_proc_scheduler, this);
	thread_started = true;
}

void multcher::downloader::join_working_thread()
{
	shutdown_asap = true;
	queue_scheduler.signal();
	pthread_join(th_scheduler, 0);

	queue_fetcher.signal();
	pthread_join(th_fetcher, 0);
}

static void do_select_job(CURLM* cmh)
{
	fd_set fdread;
	fd_set fdwrite;
	fd_set fdexcep;
	int maxfd = -1;
	long curl_timeo = -1;

	FD_ZERO(&fdread);
	FD_ZERO(&fdwrite);
	FD_ZERO(&fdexcep);

	struct timeval timeout;
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;

	curl_multi_timeout(cmh, &curl_timeo);
	if (curl_timeo >= 0)
	{
		timeout.tv_sec = curl_timeo / 1000;
		if (timeout.tv_sec > 1)
		{
			timeout.tv_sec = 1;
		}
		else
		{
			timeout.tv_usec = (curl_timeo % 1000) * 1000;
		}
	}

	curl_multi_fdset(cmh, &fdread, &fdwrite, &fdexcep, &maxfd);

	select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);
}

void multcher::downloader::handle_result_message(CURLMsg* msg)
{
	multcher_internal_t& p = internal_data[msg->easy_handle];

	_sch.set_finished_ok(p.req.domain);

	if (p.req.is_internal)
	{
		if ((msg->data.result == 0) || (msg->data.result == 22))
		{
			rtxt_consumer.receive(p.req, p.resp, msg->data.result);
		}
		else
		{
			rtxt_consumer.completely_failed(p.req);
		}

		domain_unknown_requests_t durls;
		pthread_mutex_lock(&unknown_urls_mutex);
		unknown_urls[p.req.domain].swap(durls);
		unknown_urls.erase(p.req.domain);
		pthread_mutex_unlock(&unknown_urls_mutex);

		domain_unknown_requests_t::const_iterator it;
		for (it = durls.begin(); it != durls.end(); ++it)
		{
			add_request(*it);
		}
	}
	else
	{
		consumer->receive(p.req, p.resp, msg->data.result);
	}

	if (p.additional_headers) curl_slist_free_all(p.additional_headers);
	internal_data.erase(msg->easy_handle);
	curl_easy_cleanup(msg->easy_handle);
}

void multcher::downloader::working_thread_proc_fetcher()
{
	queries_running = -1;

	while (!fetcher_wont_send_more || !shutdown_asap || queries_running || queue_fetcher.size() != 0)
	{
		add_requests((0 == queries_running) && !shutdown_asap);

		curl_multi_perform(cmh, &queries_running);

		do_select_job(cmh);

		CURLMsg* msg = 0;
		int msgs_in_queue = 0;
		while ((msg = curl_multi_info_read(cmh, &msgs_in_queue)))
		{
			handle_result_message(msg);
		}
	}
}

void multcher::downloader::working_thread_proc_scheduler()
{
	std::vector<request_t> delayed_reqs;

	while (!shutdown_asap || !delayed_reqs.empty() || queue_scheduler.size() || queries_running || queue_fetcher.size())
	{
		if (!delayed_reqs.empty())
		{
			std::vector<request_t>::iterator it;
			for (it = delayed_reqs.begin(); it != delayed_reqs.end();)
			{
				unsigned w = _sch.wait_start_download(it->domain);
				if (!w)
				{
					queue_fetcher.add(*it);
					it = delayed_reqs.erase(it);
				}
				else
				{
					++it;
				}
			}
		}

		request_t r;

		int rc = queue_scheduler.get(r, delayed_reqs.empty() && !shutdown_asap);

		if (rc >= 0)
		{
			unsigned w = _sch.wait_start_download(r.domain);
			if (w)
			{
				delayed_reqs.push_back(r);
			}
			else
			{
				queue_fetcher.add(r);
			}
		}
		else
		{
			if (shutdown_asap && !queries_running && !queue_fetcher.size() && !queue_scheduler.size() && delayed_reqs.empty()) fetcher_wont_send_more = true;
			sleep(1);
		}
	}

	fetcher_wont_send_more = true;
}












