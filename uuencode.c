/*
 * UNG's Not GNU
 *
 * Copyright (c) 2011-2019, Jakob Kaivo <jkk@ung.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#define _XOPEN_SOURCE 700
#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static const unsigned char b64s[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static inline int encodechunk(const unsigned char *in, unsigned char *out, int n, int b64)
{
	out[0] = (in[0] >> 2) & 0x3f;
	out[1] = ((in[0] << 4) | ((in[1] >> 4) & 0xf)) & 0x3f;
	out[2] = ((in[1] << 2) | ((in[2] >> 6) & 0x3)) & 0x3f;
	out[3] = (in[2]) & 0x3f;

	for (int i = 0; i < 4; i++) {
		out[i] = b64 ? b64s[out[i]] : out[i] + 0x20;
	}

	if (n < 2) {
		out[2] = b64 ? '=' : ((in[0] << 4) & 0x3f) + 0x20;
	}

	if (n < 3) {
		out[3] = b64 ? '=' : ((in[1] << 2) & 0x3f) + 0x20;
	}

	return b64 ? 4 : 4 - (3-n);
}

static int encode(FILE * in, const char *path, mode_t mode, int b64)
{
	printf("begin%s %03o %s\n", b64 ? "-base64" : "", mode, path);

	int lpos = 0;
	int linelength = b64 ? 75 : 43;

	int nread = 0;
	unsigned char ibuf[3] = {0};
	unsigned char obuf[76] = {0};

	while ((nread = fread(ibuf, 1, sizeof(ibuf), in)) != 0) {
		if (nread < 0) {
			perror("uuencode");
			return 1;
		}

		lpos += encodechunk(ibuf, obuf + lpos, nread, b64);

		if (lpos > linelength || nread < 3) {
			if (!b64) {
				fputc(0x20 + lpos, stdout);
			}

			fwrite(obuf, 1, lpos, stdout);
			fputc('\n', stdout);
			lpos = 0;
			memset(obuf, 0, sizeof(obuf));
		}
		memset(ibuf, 0, sizeof(ibuf));
	}

	printf("%s\n", b64 ? "====" : "end");
	return 0;
}

int main(int argc, char *argv[])
{
	int mime_base64 = 0;
	mode_t mode = 0644;
	FILE *input = stdin;
	int c;

	setlocale(LC_ALL, "");

	while ((c = getopt(argc, argv, "m")) != -1) {
		switch (c) {
		case 'm':
			mime_base64 = 1;
			break;

		default:
			return 1;
		}
	}

	if (optind > argc) {
		fprintf(stderr, "uuencode: missing operand\n");
		return 1;
	}

	if (argc - optind == 2) {
		input = fopen(argv[optind], "r");
		if (input == NULL) {
			fprintf(stderr, "uuencode: %s: %s\n", argv[optind], strerror(errno));
			return 1;
		}

		struct stat st;
		if (fstat(fileno(input), &st) == 0) {
			mode = st.st_mode & 0777;
		}
	}

	return encode(input, argv[argc - 1], mode, mime_base64);
}
