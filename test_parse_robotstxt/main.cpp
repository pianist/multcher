#include <multcher/robotstxt.hpp>
#include <errno.h>
#include <string.h>
#include <coda/system.h>

void test_url(const multcher::domain_robotstxt_t& rtxt, const char* url)
{
	std::string s(url);
	printf("%s: %s\n", rtxt.check_uri(s) ? "A" : "D", url);
}

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		fprintf(stderr, "Usage: ./test_parse_robotstxt robots.txt\n");
		return -1;
	}

	coda_strt s;
	if (coda_mmap_file(&s, argv[1]) < 0)
	{
		fprintf(stderr, "Can't mmap %s: (%d) %s\n", argv[1], errno, strerror(errno));
		return -1;
	}

	std::string src(s.data, s.size);
	multcher::domain_robotstxt_t rtxt;

	rtxt.parse_from_source(src.c_str(), "bla");

	while (!feof(stdin))
	{
		char buf[1024];
		if (!fgets(buf, 1024, stdin)) break;

		char* nl = strpbrk(buf, "\r\n");
		if (nl) *nl = 0;
		test_url(rtxt, buf);
	}

	return 0;
}

