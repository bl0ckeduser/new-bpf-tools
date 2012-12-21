#include <stdio.h>
#include <stdlib.h>
#include "SDL.h"

#define clean_exit(X) exit_code = X; goto clean_up;

/* getpixel() is from the SDL docs */

/*
 * Return the pixel value at (x, y)
 * NOTE: The surface must be locked before calling this!
 */
Uint32 getpixel(SDL_Surface *surface, int x, int y)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch(bpp) {
    case 1:
        return *p;

    case 2:
        return *(Uint16 *)p;

    case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
            return p[0] << 16 | p[1] << 8 | p[2];
        else
            return p[0] | p[1] << 8 | p[2] << 16;

    case 4:
        return *(Uint32 *)p;

    default:
        return 0;       /* shouldn't happen, but avoids warnings */
    }
}

int isnumstring(char *str) {
	char *p;
	for (p = str; *p; ++p)
		if (!isdigit(*p))
			return 0;
	return 1;
}

int main(int argc, char** argv)
{
	SDL_Surface *image, *mask;
	SDL_Surface *surf;
	FILE* out, *in;
	Uint8 r, g, b;
	Uint32 pixel;
	char line[1024];
	int i, j, k;
	int opcode;
	char* oper;
	char spec[1024];
	char* arg;
	char* str;
	int exit_code = 0;

	char *oper_names[256] = {NULL};
	int arg_count[256];

	oper_names[10] = "Do";
	arg_count[10] = 4;

	oper_names[12] = "PtrTo";
	arg_count[12] = 1;

	oper_names[13] = "PtrFrom";
	arg_count[13] = 1;

	oper_names[31] = "bPtrTo";
	arg_count[31] = 3;

	oper_names[35] = "zbPtrTo";
	arg_count[35] = 3;

	oper_names[14] = "BoolDie";
	arg_count[14] = 1;

	oper_names[20] = "Draw";
	arg_count[20] = 8;

	oper_names[21] = "vDraw";
	arg_count[21] = 8;

	oper_names[30] = "OutputDraw";
	arg_count[30] = 0;


	oper_names[25] = "mx";
	oper_names[26] = "my";
	oper_names[50] = "cmx";
	oper_names[51] = "cmx";
	arg_count[25] = arg_count[26] 
	= arg_count[50] = arg_count[51] 
	= arg_count[25] = 1;

	oper_names[15] = "Wait";
	arg_count[15] = 1;

	oper_names[11] = "Echo";
	arg_count[11] = 1;
	oper_names[255] = "End";
	arg_count[255] = 0;

	if (argc != 5) {
		printf("Usage: %s graphics.bmp mask.bmp source.asm out.BPF\n",
			*argv);
		exit(1);
	}

	image = SDL_LoadBMP(argv[1]);
	mask = SDL_LoadBMP(argv[2]);

	if (!(out = fopen(argv[4], "w"))) {
		printf("Could not open output binary\n");
		exit(1);
	}

	if (!(in = fopen(argv[3], "r"))) {
		printf("Could not open source code\n");
		exit(1);
	}

	if (!image || !mask) {
		printf("Graphics loading failed\n");
		exit(1);
	}

	if(image->w < 255 || image->h < 255
	|| mask->w < 255 || mask->h < 255) {
		printf("Graphics wrong size\n");
		exit(1);
	}

	/* write graphics data */
	for (k = 0; k < 2; k++) {
		for (i = 0; i < 255; i++) {
			for (j = 0; j < 255; j++) {
				surf = k ? mask : image;
				pixel = getpixel(surf, i, j);
				SDL_GetRGB(pixel, surf->format, &r, &g, &b);
				fputc(r, out);
				fputc(g, out);
				fputc(b, out);
			}
		}
	}

	while (!feof(in)) {
		fgets(line, 1024, in);

		/* skip empty or comment lines */
		if (*line == '\n' || *line==';' || strlen(line) < 3)
			continue;

		/* prune newline */
		if (line[strlen(line) - 1] == '\n')
			line[strlen(line) - 1] = 0;

		/* stop reading at tab or semicolon */
		for (i = 0; i < strlen(line); i++)
			if (line[i] == ';' || line[i] == '\t') {
				line[i] = 0;
				break;
			}

		/* get opcode */
		oper = strtok(line, " ");
		opcode = -1;
		for (i = 0; i < 255; i++)
			if (oper_names[i] && !strcmp(oper_names[i], oper)) {
				opcode = i;
				break;
			}
		if (opcode == -1) {
			printf("Unknown operator '%s'\n", oper);
			clean_exit(1);
		}
		fputc(opcode, out);

		/* get args */
		for (i = 0; i < arg_count[opcode]; i++) {
			arg = strtok(NULL, " ");
			if (!arg || !isnumstring(arg) || atoi(arg) > 255 || atoi(arg) < 0) {
				printf("Illegal data\n");
				clean_exit(1);
			}
			fputc(atoi(arg), out);
		}
	}

	/* final "End" opcode */
	fputc(255, out);

clean_up:
	fclose(out);
	fclose(in);
	SDL_FreeSurface(image);
	SDL_FreeSurface(mask);

	return exit_code;
}
