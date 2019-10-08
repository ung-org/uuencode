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
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static const char *b64s =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int encode(FILE * in, char *out, int mode, int b64)
{
	char bin[3], bout[76];
	int nread = 0, lread = 0, lpos = 0;
	int linelength = b64 ? 54 : 43;
	int i;

	printf("begin%s %03o %s\n", b64 ? "-base64" : "", mode, out);

	while (!feof(in)) {
		nread = fread(bin, sizeof(char), 3, in);
		if (nread > 0) {
			bout[lpos++] = (bin[0] >> 2) & 0x3f;
			bout[lpos++] =
			    ((bin[0] << 4) | ((bin[1] >> 4) & 0xf)) & 0x3f;
			bout[lpos++] =
			    ((bin[1] << 2) | ((bin[2] >> 6) & 0x3)) & 0x3f;
			bout[lpos++] = (bin[2]) & 0x3f;
			bin[0] = bin[1] = bin[2] = 0;
			lread += nread;
		}

		if (feof(in) || nread == 0 || lread > linelength) {
			for (i = 0; i < lpos; i++)
				bout[i] = b64 ? b64s[bout[i]] : bout[i] + 0x20;

			if (b64) {
				if (nread == 1)
					bout[lpos - 2] = '=';
				if (nread == 1 || nread == 2)
					bout[lpos - 1] = '=';
			}

			if (!b64)
				putchar(0x20 + lread);
			fwrite(bout, sizeof(char), lpos, stdout);
			fputc('\n', stdout);
			lread = 0;
			lpos = 0;
		}
	}

	if (b64)
		printf("====\n");
	else
		printf("%c\nend\n", 0x20);

	return 0;
}

int main(int argc, char *argv[])
{
	int b64 = 0, mode = 0644, c;
	FILE *input;
	struct stat st;

	while ((c = getopt(argc, argv, ":m")) != -1) {
		switch (c) {
		case 'm':
			b64 = 1;
			break;
		default:
			return 1;
		}
	}

	if (argc - optind == 2) {
		input = fopen(argv[optind++], "r");
		if (fstat(fileno(input), &st) == 0)
			mode = st.st_mode & 0777;
	} else if (argc - optind == 1)
		input = stdin;
	else
		return 1;

	if (input == NULL)
		return 1;	// File I/O error

	return encode(input, argv[optind], mode, b64);
}
