#include <stdio.h>

#include "arg.h"
#include "util.c"

#define VERSION "0.1"

static void dumpwave(float *wave, size_t wsize);
static size_t readf32le(char *filename, float **wave);
static void usage(void);

char *argv0;

static void
dumpwave(float *wave, size_t wsize)
{
	float *wptr = wave + 1;
	while (wsize--)
		printf("[%6ld]: %f\n",
				wave - wptr, *(wave++));
}

static size_t
readf32le(char *filename, float **wave)
{
	FILE *fp = NULL; /* wave *file */
	int ch[4]; /* temporary buffer */
	size_t wsize = 0; /* wave size */

	union {
		float f;
		char carr[4];
	} fcarr; /* union that converts 4 chars from file to float */

	if ((fp = fopen(filename, "r")) == NULL) {
		die("unable to open %s:", filename);
	} /* opening file */

	*wave = malloc(0);

	while ((ch[0] = fgetc(fp), /* this code will work only on | 0 -> 3 */
			ch[1] = fgetc(fp), /* little endian architectures | 1 -> 2 */
			ch[2] = fgetc(fp), /* like x86, on big endian you | 2 -> 1 */
			ch[3] = fgetc(fp)) != -1) { /* must reverse array | 3 -> 0 */
		fcarr.carr[0] = (int)ch[0]; fcarr.carr[1] = (int)ch[1];
		fcarr.carr[2] = (int)ch[2]; fcarr.carr[3] = (int)ch[3];
		*wave = realloc(*wave, sizeof(float) * ++wsize);
		(*wave)[wsize - 1] = fcarr.f;
	}

	fclose(fp);

	return wsize; /* return samples count */
}

static void
usage(void)
{
	die("usage: %s [-v] [-f waveformat] wave", argv0);
}

int
main(int argc, char *argv[])
{
	char *format = "f32le"; /* by default cbms reads stream as f32le wave */
	float *wave = NULL;
	size_t wsize = 0;

	ARGBEGIN {
	case 'v':
		die("cbms-"VERSION, argv0); break;
	case 'f':
		format = ARGF(); break;
	default:
		usage(); break;
	} ARGEND

	if (argc != 1)
		usage();

	if (!strcmp(format, "f32le"))
		wsize = readf32le(argv[0], &wave);
	else
		die("unknown wave format [check -f parameter]");

	dumpwave(wave, wsize);
	free(wave);
}
