#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "passwdqc.h"

int main(int argc, const char **argv)
{
	passwdqc_params_t params;
	char *reason, *pass;

	if (argc > 1 && argv[1][0] == '-') {
		if (!strcmp("-h", argv[1]) || !strcmp("--help", argv[1])) {
			printf("Generate quality controllable passphrase.\n"
			    "\nUsage: pwqgen [options]\n"
			    "\nValid options are:\n"
			    "  random=N\n"
			    "       set size of randomly-generated passphrase in bits;\n"
			    "  config=FILE\n"
			    "       Load config FILE in passwdqc.conf format;\n"
			    "  --version\n"
			    "       print program version and exit;\n"
			    "  -h or --help\n"
			    "       print this help text and exit.\n");
			return 0;
		}

		if (!strcmp("--version", argv[1])) {
			printf("pwqgen version %s\n", PASSWDQC_VERSION);
			exit(EXIT_SUCCESS);
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
	if (!pass) {
		fprintf(stderr, "pwqgen: Failed to generate a passphrase.\n"
		    "This could happen for a number of reasons: you could have requested\n"
		    "an impossible passphrase length, or the access to kernel random number\n"
		    "pool could have failed.\n");
		return 1;
	}
	puts(pass);
	memset(pass, 0, strlen(pass));
	free(pass);
	pass = 0;
	return 0;
}
