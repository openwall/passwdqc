/*
 * Copyright (c) 2008,2009 by Dmitry V. Levin
 * Copyright (c) 2016,2021 by Solar Designer
 * See LICENSE
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "passwdqc.h"

static void
print_help(void)
{
	puts("Generate quality controllable passphrase.\n"
	    "\nUsage: pwqgen [options]\n"
	    "\nValid options are:\n"
	    "  random=N\n"
	    "       set size of randomly-generated passphrase in bits;\n"
	    "  config=FILE\n"
	    "       load config FILE in passwdqc.conf format;\n"
	    "  --version\n"
	    "       print program version and exit;\n"
	    "  -h or --help\n"
	    "       print this help text and exit.");
}

int main(int argc, const char **argv)
{
	passwdqc_params_t params;
	char *reason, *pass;
	int retval;

	if (argc > 1 && argv[1][0] == '-') {
		if (!strcmp("-h", argv[1]) || !strcmp("--help", argv[1])) {
			print_help();
			return 0;
		}

		if (!strcmp("--version", argv[1])) {
			printf("pwqgen version %s\n", PASSWDQC_VERSION);
			return 0;
		}
	}

	passwdqc_params_reset(&params);
	if (argc > 1 &&
	    passwdqc_params_parse(&params, &reason, argc - 1, argv + 1)) {
		fprintf(stderr, "pwqgen: %s\n",
		    (reason ? reason : "Out of memory"));
		free(reason);
		return 1;
	}

	pass = passwdqc_random(&params.qc);
	passwdqc_params_free(&params);
	if (!pass) {
		fprintf(stderr, "pwqgen: Failed to generate a passphrase.\n"
		    "This could happen for a number of reasons: you could have requested\n"
		    "an impossible passphrase length, or the access to kernel random number\n"
		    "pool could have failed.\n");
		return 1;
	}

	setvbuf(stdout, NULL, _IONBF, 0);

	retval = (puts(pass) >= 0 && fflush(stdout) == 0) ? 0 : 1;

	_passwdqc_memzero(pass, strlen(pass));
	free(pass);

	return retval;
}
