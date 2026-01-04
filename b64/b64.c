/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <r9k/argparse.h>
#include <r9k/panic.h>
#include <r9k/ioutils.h>
#include <r9k/string.h>

#include "base64.h"

static int encode(struct argparse *ap, struct option *e)
{
        __attr_ignore(e);

	char *origin = NULL;
        const char *plain = argparse_val(ap, 0);
	if (!plain) {
		origin = readall(stdin);
		plain = trim(origin);
	}

        char *cipher = base64_encode((unsigned char *) plain, strlen(plain));
	PANIC_IF(!cipher, "error: no memory\n");

	if (argparse_has(ap, "u")) {
		size_t n = strlen(cipher);
		for (size_t i = 0; i < n; i++) {
			if (cipher[i] == '+')
				cipher[i] = '-';
			else if (cipher[i] == '/')
				cipher[i] = '_';
		}
	}

        printf("%s\n", cipher);
        free(cipher);

	if (origin)
		free(origin);

        return 0;
}

static int decode(struct argparse *ap, struct option *e)
{
	__attr_ignore(e);

	char *origin = NULL;
	const char *srcptr = argparse_val(ap, 0);
	if (!srcptr) {
		origin = readall(stdin);
		srcptr = trim(origin);
	}

	size_t n = strlen(srcptr);
	char *cipher = malloc(n + 4);

	memcpy(cipher, srcptr, n);
	cipher[n] = '\0';

	if (argparse_has(ap, "u")) {
		for (size_t i = 0; i < n; i++) {
			if (cipher[i] == '-') cipher[i] = '+';
			else if (cipher[i] == '_') cipher[i] = '/';
		}

		size_t pad = n & 3;
		if (pad == 1)
			PANIC("error: invalid url safe base64\n");

		if (pad == 2) {
			cipher[n++] = '=';
			cipher[n++] = '=';
		} else if (pad == 3) {
			cipher[n++] = '=';
		}
		cipher[n] = '\0';
	}

	size_t size;
	unsigned char *plain = base64_decode(cipher, &size);
	PANIC_IF(!plain, "error: invalid base64\n");
	fwrite(plain, 1, size, stdout);
	putchar('\n');

	free(cipher);
	free(plain);

	if (origin)
		free(origin);

	return 0;
}

int main(int argc, char* argv[])
{
        struct argparse *ap;
        struct option *e, *d, *u;

        ap = argparse_create("b64", "1.0");
        PANIC_IF(!ap, "error: argparse initialize failed");

        argparse_add0(ap, &e, "e", NULL, "encode", encode, 0);
        argparse_add0(ap, &d, "d", NULL, "decode", decode, 0);
        argparse_add0(ap, &u, "u", NULL, "url safe", NULL, 0);
        argparse_add1(ap, &u, "o", NULL, "output file", "PATH", NULL, O_REQUIRED);

        argparse_mutual_exclude(ap, &e, &d);

        if (argparse_run(ap, argc, argv) != 0)
                PANIC("%s\n", argparse_error(ap));

        argparse_destroy(ap);

        return 0;
}