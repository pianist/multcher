#include "multcher.hpp"
#include <string.h>
#include <stdlib.h>

multcher::multcher()
  : shutdown_asap(false)
  , thread_started(false)
  , max_concur(10)
  , consumer(0)
{
	cmh = curl_multi_init();
}

multcher::~multcher()
{
	curl_multi_cleanup(cmh);
}

void multcher::add_request(const multcher_request& req)
{
	queue.add(req);
}

static size_t cb_response_data(char* ptr, size_t size, size_t nmemb, void* userdata)
{
	multcher_response* resp = (multcher_response*)userdata;
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
	multcher_response* resp = (multcher_response*)userdata;
	std::string h;

	if (strncasecmp((char*)ptr, "HTTP/", 5) == 0)
	{
		resp->code = atoi((char*)ptr + 9);
		return nmemb;
	}

	h = check_header((char*)ptr, size * nmemb, "Location: ");
	if (!h.empty())
	{
		multcher_redirect redir;
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

void multcher::add_requests(bool can_lock)
{
	multcher_request r;

	while (queue.get(r, can_lock) >= 0)
	{
		can_lock = false;

		CURL* ch = curl_easy_init();
		multcher_internal& p = internal_data[ch];
		p.req = r;

		curl_easy_setopt(ch, CURLOPT_URL, r.url.c_str());
		curl_easy_setopt(ch, CURLOPT_FOLLOWLOCATION, 1);
		curl_easy_setopt(ch, CURLOPT_MAXREDIRS, 3);
		curl_easy_setopt(ch, CURLOPT_WRITEFUNCTION, cb_response_data);
		curl_easy_setopt(ch, CURLOPT_WRITEDATA, &p.resp);
		curl_easy_setopt(ch, CURLOPT_HEADERFUNCTION, cb_response_header);
		curl_easy_setopt(ch, CURLOPT_HEADERDATA, &p.resp);
		curl_easy_setopt(ch, CURLOPT_ENCODING, "gzip,deflate");

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

static void* job_working_thread_proc(void* p)
{
	multcher* m = (multcher*)p;
	m->working_thread_proc();
	return 0;
}

void multcher::create_working_thread()
{
	pthread_create(&th_fetcher, 0, job_working_thread_proc, this);
	thread_started = true;
}

void multcher::join_working_thread()
{
	shutdown_asap = true;
	queue.signal();
	pthread_join(th_fetcher, 0);
}

void multcher::working_thread_proc()
{
	queries_running = -1;

	while (!shutdown_asap || queries_running)
	{
		add_requests(0 == queries_running);

		curl_multi_perform(cmh, &queries_running);

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

		CURLMsg* msg = 0;
		int msgs_in_queue = 0;
		while ((msg = curl_multi_info_read(cmh, &msgs_in_queue)))
		{
			multcher_internal& p = internal_data[msg->easy_handle];

			consumer->receive(p.req, p.resp, msg->data.result);

			if (p.additional_headers) curl_slist_free_all(p.additional_headers);
			internal_data.erase(msg->easy_handle);
			curl_easy_cleanup(msg->easy_handle);
		}

		if (!thread_started && !queries_running && queue.size() == 0)
		{
			shutdown_asap = true;
		}
	}
}











