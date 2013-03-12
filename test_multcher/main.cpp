#include <stdio.h>
#include <multcher/multcher.hpp>
#include <string.h>

struct MyConsumer : public multcher::consumer_t
{
	MyConsumer();
	~MyConsumer() throw();
	void receive(const multcher::request_t& req, const multcher::response_t& resp, CURLcode code);
};

MyConsumer::MyConsumer()
{
}

MyConsumer::~MyConsumer() throw()
{
}

void MyConsumer::receive(const multcher::request_t& req, const multcher::response_t& resp, CURLcode code)
{
	fprintf(stdout, "--->8---\n");
	fprintf(stdout, "Request complete with code = %d\n", code);
	fprintf(stdout, "URL: %s\n", req.url.c_str());
	for (multcher::redirects_t::const_iterator it = resp.redirects.begin(); it != resp.redirects.end(); ++it)
		fprintf(stderr, "\t(%d) %s\n", it->code, it->location.c_str());
	fprintf(stdout, "HTTP code: %d\n", resp.code);
	fprintf(stdout, "Content-type: %s\n", resp.content_type.c_str());
	fprintf(stdout, "---8<---\n");
}

int main(int argc, char** argv)
{
	multcher::downloader mym("SuperFetcher");
	MyConsumer cons;
	mym.set_consumer(&cons);

	mym.create_working_thread();

	if ((argc > 1) && (strcasecmp(argv[1], "stdin") == 0))
	{
		while (!feof(stdin))
		{
			char url[1024];
			if (fgets(url, 1024, stdin))
			{
				char* nl = strpbrk(url, "\r\n");
				if (nl) *nl = 0;

				multcher::request_t r;
				r.url = url;
				r.accept_language = "en";
				mym.add_request(r);
			}
		}
	}
	else
	{
		multcher::request_t r;
		r.url = "http://yandex.ru:80/";
		mym.add_request(r);

		r.url = "https://mail.ru";
		mym.add_request(r);

		r.url = "http://busuu.com/";
		r.accept_language = "ru";
		mym.add_request(r);

		r.url = "https://vk.com/pianisteg";
		r.fail_on_error = true;
		mym.add_request(r);
	}

	mym.join_working_thread();

	return 0;
}

