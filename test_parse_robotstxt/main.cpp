#include <multcher/robotstxt.hpp>
#include <errno.h>
#include <string.h>
#include <coda/system.h>

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

	

	return 0;
}

