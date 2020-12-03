#include <stdio.h>
#include <stdlib.h>

#include "arg.h"
#include "util.c"

#define VERSION "0.1"

typedef struct {
	char *name;
	float *wave;
	size_t wsize;
	int sampleRate, channels;
} Wave;

static void dumpwave(Wave wave);
static Wave readf32(char *filename, char endianness, int sampleRate, int channels);
static void savef32(char *filename, Wave wave, char endianness);
static void shell(void);
static float wavelength(size_t wavesize, int sampleRate, int channels);
static void wavereverse(Wave *wave, size_t beginning, size_t ending);
static void usage(void);

#include "config.h"
char *argv0;

static void
dumpwave(Wave wave)
{
	float *wptr = wave.wave + 1;
	while (wave.wsize--)
		printf("[%6ld]: %f\n",
				wave.wave - wptr, *(wave.wave++));
}

static Wave
readf32(char *filename, char endianness, int sampleRate, int channels)
{
	FILE *fp = NULL; /* wave *file */
	int ch[4]; /* temporary buffer */
	Wave ret;

	union {
		float f;
		char carr[4];
	} fcarr; /* union that converts 4 chars from file to float */

	if ((fp = fopen(filename, "r")) == NULL) {
		die("unable to open %s:", filename);
	} /* opening file */

	ret.name = filename;
	ret.wave = malloc(0);
	ret.wsize = 0;
	ret.sampleRate = sampleRate ? sampleRate : 48000;
	ret.channels = channels ? channels : 1;

	while ((ch[endianness ? 3 : 0] = fgetc(fp), /* this code will work only on | ? 0 : 3] = | */
			ch[endianness ? 2 : 1] = fgetc(fp), /* little endian architectures | ? 1 : 2] = | */
			ch[endianness ? 1 : 2] = fgetc(fp), /* like x86, on big endian you | ? 2 : 1] = | */
			ch[endianness ? 0 : 3] = fgetc(fp)) != -1) { /* must reverse array | ? 3 : 0] = | */
		fcarr.carr[0] = (int)ch[0]; fcarr.carr[1] = (int)ch[1];
		fcarr.carr[2] = (int)ch[2]; fcarr.carr[3] = (int)ch[3];
		ret.wave = realloc(ret.wave, sizeof(float) * ++(ret.wsize));
		(ret.wave)[ret.wsize - 1] = fcarr.f;
	}

	fclose(fp);

	return ret;
}

static void
savef32(char *filename, Wave wave, char endianness)
{
	FILE *fp = NULL; /* wave *file */
	union {
		char carr[4];
		float f;
	} carrf; /* union that converts float to 4 chars array */

	if ((fp = fopen(filename, "w")) == NULL) {
		die("unable to open %s:", filename);
	} /* opening file */

	while (wave.wsize--) {
		carrf.f = *(wave.wave++);
		fprintf(fp, "%c%c%c%c",
				carrf.carr[endianness ? 3 : 0], carrf.carr[endianness ? 2 : 1],
				carrf.carr[endianness ? 1 : 2], carrf.carr[endianness ? 0 : 3]);
	}

	fclose(fp);
}

static void
shell(void)
{
	while (1);
}

static float
wavelength(size_t wsize, int sampleRate, int channels)
{
	return (float)wsize / sampleRate / channels;
}

static void
wavereverse(Wave *wave, size_t beginning, size_t ending)
{
	size_t bufsiz = ending - beginning;
	float *wavebuf = malloc(sizeof(float) * bufsiz);
	size_t iter = -1;
	while (++iter < bufsiz)
		wavebuf[iter] = wave->wave[beginning + iter];
	iter = -1; ++ending;
	while (ending-- > beginning) {
		wave->wave[(iter + beginning) - 1] = wavebuf[bufsiz - ++iter];
	}
	free(wavebuf);
}

static void
usage(void)
{
	die("usage: %s [-v] [-f waveformat] wave", argv0);
}

int
main(int argc, char *argv[])
{
	char *format = "f32le"; /* by default med reads stream as f32le wave, */
	int sampleRate = 48000; /* default sample rate is 48 kHz */
	int channels = 1;       /* and there is 1 channel. */
	Wave wave;

	ARGBEGIN {
	case 'v':
		die("med-"VERSION, argv0); break;
	case 'f':
		format = ARGF(); break;
	case 's':
		sampleRate = (int)strtol(ARGF(), NULL, 10); break;
	case 'c':
		channels = (int)strtol(ARGF(), NULL, 10); break;
	default:
		usage(); break;
	} ARGEND

	if (argc != 1)
		usage();

	if (!strcmp(format, "f32le"))
		wave = readf32(argv[0], 0, sampleRate, channels);
	else if (!strcmp(format, "f32be"))
		wave = readf32(argv[0], 1, sampleRate, channels);
	else
		die("unknown wave format [check -f parameter]");

	printf("==================\nloaded [%s].\nsample rate: %d.\nwave length: %fs.\n",
			argv[0], wave.sampleRate, wavelength(wave.wsize, wave.sampleRate, wave.channels));

	free(wave.wave);
}
