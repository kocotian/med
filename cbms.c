#include <stdio.h>

#include "arg.h"
#include "util.c"

#define VERSION "0.1"

char *argv0;

static void
usage(void)
{
	die("usage: %s [-v]", argv0);
}

static void
version(void)
{
	die("cbms-"VERSION, argv0);
}

int
main(int argc, char *argv[])
{
	ARGBEGIN {
	case 'v':
		version(); break;
	default:
		usage(); break;
	} ARGEND
}
