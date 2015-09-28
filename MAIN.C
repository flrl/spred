#include <stdio.h>
#include <dos.h>
#include <stdlib.h>

#include <sys/stat.h>

#include "sprite.h"
#include "ui.h"

Sheet sheet;

struct options {
	unsigned spr_width;
	unsigned spr_height;
	unsigned sh_width;
	unsigned sh_height;
	unsigned pal_colors;
};

static int get_options(struct options *options, int argc, char **argv) {
	int i, c, v;

	if (argc < 2) {
		fprintf(stderr, "not enough arguments\n");
		return -1;
	}

	for (i=1; i < argc; i++) {
		if (argv[i][0] == '/') {
			c = argv[i][1];
			v = atoi(&argv[i][2]);
			if (v < 0) {
				fprintf(stderr, "invalid %c option: %d\n", c, v);
				return -1;
			}
			switch (c) {
				case 'w':
					options->spr_width = v;
					break;
				case 'h':
					options->spr_height = v;
					break;
				case 'W':
					options->sh_width = v;
					break;
				case 'H':
					options->sh_height = v;
					break;
				case 'C':
					options->pal_colors = v;
					break;
				default:
					fprintf(stderr, "unrecognised option: %c\n", c);
					break;
			}
		}
		else {
			break;
		}
	}

	return i;
}

int main(int argc, char **argv) {
	struct options options = { 8, 8, 8, 8, 16 };
	int r;
	char *fname;
	struct stat stat_buf;

	r = get_options(&options, argc, argv);
	if (r < 0) return -1;
	fname = argv[r];

	if (0 == stat(fname, &stat_buf)) {
		fprintf(stderr, "loading %s...\n", fname);

		r = sheet_read(fname, &sheet);
		if (r) {
			fprintf(stderr, "failed\n");
			return -1;
		}
	}
	else {
		fprintf(stderr,
			"create %s containing %ux%u %ux%u sprites...\n",
			fname,
			options.sh_width, options.sh_height,
			options.spr_width, options.spr_height);

		r = sheet_init(&sheet, options.sh_width, options.sh_height,
			options.spr_width, options.spr_height,
			options.pal_colors);
		if (r) {
			fprintf(stderr, "failed\n");
			return -1;
		}
	}

	sleep(1); /* so you get to see the message */

	vga13h_init();

	ui_13(&sheet);

	vga13h_done();

	return 0;
}