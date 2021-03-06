#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arg.h"
#include "util.c"

#define VERSION "0.1"

typedef struct {
	char *name;
	float *wave;
	size_t wsize, leftSelection, rightSelection;
	int sampleRate, channels;
	char modificated;
} Wave;

static void changewavselection(Wave *wave, char isRight, char *l);
static void docommand(Wave **waves, size_t *waven, int *selwav, char *l);
static void editwave(Wave **waves, size_t *waven, char *wname);
static void newwave(Wave **waves, size_t *waven, char *wname);
static void playwave(Wave wave);
static void printwaveinfo(Wave wave);
static void printwavelist(Wave *waves, size_t waven);
static Wave readf32(char *filename, char endianness, int sampleRate, int channels);
static void savef32(char *filename, Wave wave, char endianness);
static void selectwave(Wave *waves, size_t waven, int *selwav, char *l);
static void shell(Wave **waves, size_t *waven);
static void wavedump(Wave wave);
static void wavevolume(Wave *wave, char *l);
static float wavelength(size_t wavesize, int sampleRate, int channels);
static void wavereverse(Wave *wave);
static void writewave(Wave wave, char *name);
static void usage(void);

#include "config.h"
char *argv0;

static void
changewavselection(Wave *wave, char isRight, char *l)
{
	size_t *val = isRight ? &(wave->rightSelection) : &(wave->leftSelection);
	char secs = (l[strlen(l) - 1] == 's');
	switch (*l) {
	case '=': /* set to X number of samples/seconds */
		*val = strtol(++l, NULL, 10) * wave->channels * (secs ? wave->sampleRate : 1);
		break;
	case '+': /* add X number of samples/seconds */
		*val += strtol(++l, NULL, 10) * wave->channels * (secs ? wave->sampleRate : 1);
		break;
	case '-': /* remove X number of samples/seconds */
		*val -= strtol(++l, NULL, 10) * wave->channels * (secs ? wave->sampleRate : 1);
		break;
	default:
		printf("err: undefined char: %c\n", *l);
		break;
	}
}

static void
docommand(Wave **waves, size_t *waven, int *selwav, char *l)
{
	if (*selwav < 0)
		puts("err: no selected wave");
	else if(!strcmp("dump", l))
		wavedump((*waves)[*selwav]);
	else if(!strcmp("rev", l))
		wavereverse(&((*waves)[*selwav]));
	else if(!strcmpt("vol/", l, '/'))
		wavevolume(&((*waves)[*selwav]), l + 4);
	else
		puts("?");
}

static void
editwave(Wave **waves, size_t *waven, char *wname)
{
	size_t ls = 0;
	*waves = realloc(*waves, sizeof(Wave) * ++(*waven));
	if (*wname == '\0')
		printf("filename: "), ls = getline(&wname, &ls, stdin);
	if (wname[ls - 1] == '\n') wname[ls - 1] = '\0';
	(*waves)[(*waven) - 1] = readf32(wname, 0, 48000, 2);
	(*waves)[(*waven) - 1].name = wname;
	(*waves)[(*waven) - 1].leftSelection =
		(*waves)[(*waven) - 1].rightSelection = -1;
	(*waves)[(*waven) - 1].modificated = 0;
}

static void
newwave(Wave **waves, size_t *waven, char *wname)
{
	size_t ls = 0, lr = 0;
	*waves = realloc(*waves, sizeof(Wave) * ++(*waven));
	if (*wname == '\0') {
		printf("name [blank for default]: ");
		if ((lr = getline(&wname, &ls, stdin)) < 2)
			wname = "[no name]";
		if (wname[lr - 1] == '\n') wname[lr - 1] = '\0';
	}
	(*waves)[(*waven) - 1].name = calloc(strlen(wname), 0);
	strncpy((*waves)[(*waven) - 1].name, wname, strlen(wname));
	(*waves)[(*waven) - 1].sampleRate = 48000;
	(*waves)[(*waven) - 1].channels = 2;
}

static void
playwave(Wave wave)
{
	char *wname, *cmd;
	wname = calloc(strlen(wave.name) + 6 /* "/tmp/" = 5, plus null
										   terminator = 6 */, 0);
	cmd = calloc(BUFSIZ, 0);
	free(cmd); /* this is really stupid, i don't know how it works,
				  without calloc and freeing memory, with NULL as value
				  of cmd, it crashes. After calloc and freeing allocated
				  memory it works, when it should not. TODO */
	strcat(wname, "/tmp/");
	strcat(wname, wave.name);
	strrep(wname + 5, '/', '_');
	savef32(wname, wave, 0);
	snprintf(cmd, BUFSIZ, "ffplay -autoexit -f f32le -ar %d -channels %d -showmode 0 %s 2> /dev/null",
			wave.sampleRate, wave.channels, wname);
	printf("playing: wave \"%s\" @ %dHz with %d channels\n",
			wave.name, wave.sampleRate, wave.channels);
	system(cmd);
	unlink(wname);
	free(wname);
}

static void
printwaveinfo(Wave wave)
{
	if (wave.name != NULL)
		printf("\"%s\":\n\
\tsample rate:      %d,\n\
\tchannels:         %d,\n\
\twave length:      %fs,\n\
\tleft selection:  +%fs,\n\
\tright selection: +%fs,\n\
\tselection size:   %fs,\n\
\tmodificated:      %s;\n",
				wave.name, wave.sampleRate, wave.channels,
				wavelength(wave.wsize, wave.sampleRate, wave.channels),
				wave.leftSelection == -1 ? 0 :
					wavelength(wave.leftSelection, wave.sampleRate, wave.channels),
				wave.rightSelection == -1 ?
					wavelength(wave.wsize, wave.sampleRate, wave.channels) :
					wavelength(wave.rightSelection, wave.sampleRate, wave.channels),
				wavelength(
					(wave.rightSelection == -1 ? wave.wsize :
						wave.rightSelection) -
					(wave.leftSelection == -1 ? 0 :
						wave.leftSelection),
					wave.sampleRate, wave.channels),
				wave.modificated ? "yes" : "no");
	else
		puts("wave is null");
}

static void
printwavelist(Wave *waves, size_t waven)
{
	Wave *ws = waves--;
	while ((++waves - ws) < waven)
		printf("[%ld]: \"%s\"\n",
			waves - ws, (*waves).name);
}

static Wave             /* 1 is bigger than 0, so 1 is big endian ;) */
readf32(char *filename, char endianness, int sampleRate, int channels)
{
	FILE *fp = NULL; /* wave *file */
	int ch[4]; /* temporary buffer */
	Wave ret;

	union {
		float f;
		char carr[4];
	} fcarr; /* union that converts 4 chars from file to float */

	if ((fp = fopen(filename, "r")) == NULL)
		die("unable to open %s:", filename); /* opening file, temporary
												dies; TODO */

	ret.name = filename;
	ret.wave = malloc(0);
	ret.wsize = ret.modificated = 0;
	ret.sampleRate = sampleRate ? sampleRate : 48000;
	ret.channels = channels ? channels : 2;
	ret.leftSelection = ret.rightSelection = -1;

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
selectwave(Wave *waves, size_t waven, int *selwav, char *l)
{
	*selwav = strtol(++l, NULL, 10);
	if (*selwav >= waven)
		printf("wave [%d] doesn't exist, unselecting\n",
				*selwav), *selwav = -1;
}

static void
shell(Wave **waves, size_t *waven)
{
	char *l; size_t lsiz = 0, lsizr = 0;
	int selwav = -1;

	l = malloc(lsiz);
	printf(":");
	while ((lsizr = getline(&l, &lsiz, stdin)) > 0) {
		if (l[lsizr - 1] == '\n') l[lsizr - 1] = '\0';
		switch (*l) {
		case '#': /* comment */
			break;
		case ':': /* command */
			docommand(waves, waven, &selwav, l + 1);
			break;
		case 'e': /* edit */
			editwave(waves, waven, *(l + 1) == ' ' ?
					l + 2 : l + 1); break;
		case 'i': /* info */
			printwaveinfo((*waves)[selwav]); break;
		case 'l': /* list */
			printwavelist(*waves, *waven); break;
		case 'n': /* new */
			newwave(waves, waven, *(l + 1) == ' ' ?
					l + 2 : l + 1); break;
		case 'p': /* play wave */
			playwave((*waves)[selwav]); break;
		case 's': /* select wave */
			selectwave(*waves, *waven, &selwav, l); break;
		case 'w': /* write */
			writewave((*waves)[selwav], *(l + 1) == ' ' ?
					l + 2 : l + 1); break;
		case 'q': /* quit */
			goto stop; break;
		case 'L': /* left selection change */
			changewavselection(&((*waves)[selwav]), 0, l + 1); break;
		case 'R': /* left selection change */
			changewavselection(&((*waves)[selwav]), 1, l + 1); break;
		default:
			puts("?"); break;
		}
		if (selwav != -1)
			printf("[wave: %d]:", selwav);
		else
			printf(":");
	}

	puts("");
	stop:
	free(l);
}

static void
wavedump(Wave wave)
{
	float *wptr = wave.wave + 1;
	while (wave.wsize--)
		printf("[%6ld]: %f\n",
				wave.wave - wptr, *(wave.wave++));
}

static void
wavevolume(Wave *wave, char *l)
{
	int iter = wave->leftSelection == -1 ? -1 : wave->leftSelection - 1;
	float voldiff;
	voldiff = strtof(l, NULL);
	while (++iter < (wave->rightSelection == -1 ? wave->wsize : wave->rightSelection))
		wave->wave[iter] *= voldiff;
}

static float
wavelength(size_t wsize, int sampleRate, int channels)
{
	return (float)wsize / sampleRate / channels;
}

static void
wavereverse(Wave *wave)
{
	size_t beginning = wave->leftSelection == -1 ? 0 : wave->leftSelection - 1,
		   ending = wave->rightSelection == -1 ? wave->wsize : wave->rightSelection;
	size_t bufsiz = ending - beginning;
	float *wavebuf = malloc(sizeof(float) * bufsiz);
	size_t iter = -1;
	while (++iter < bufsiz)
		wavebuf[iter] = wave->wave[beginning + iter];
	iter = -1; ++ending;
	while (ending-- > beginning)
		wave->wave[(iter + beginning) - 1] = wavebuf[bufsiz - ++iter];
	free(wavebuf);
}

static void
writewave(Wave wave, char *name)
{
	savef32((*name == 0 ? wave.name : name), wave, 0);
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
	int channels = 2;       /* and there are 2 channels. */
	Wave *waves;            /* this is a waves array */
	size_t waven = 0;       /* and the size of array. */
	int argx = -1;          /* iterator for files (argv) */

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

	waves = malloc(0);

	while (++argx < argc) {
		waves = realloc(waves, sizeof(Wave) * ++waven);
		if (!strcmp(format, "f32le"))
			waves[argx] = readf32(argv[argx], 0, sampleRate, channels);
		else if (!strcmp(format, "f32be"))
			waves[argx] = readf32(argv[argx], 1, sampleRate, channels);
		else
			die("unknown wave format [check -f parameter]");
	}

	shell(&waves, &waven);

	argx = -1;
	while (++argx < waven)
		free(waves[argx].wave);
	free(waves);
}
