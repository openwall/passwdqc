/*
 * Copyright (c) 2008,2009 by Dmitry V. Levin
 * Copyright (c) 2010 by Solar Designer
 * See LICENSE
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "passwdqc.h"

static void clean(char *dst, int size)
{
	if (!dst)
		return;
	memset(dst, 0, size);
	free(dst);
}

static char *read_line(unsigned int size, int eof_ok)
{
	char *p, *buf = malloc(size + 1);

	if (!buf) {
		fprintf(stderr, "pwqcheck: Memory allocation failed.\n");
		return NULL;
	}

	if (!fgets(buf, size + 1, stdin)) {
		clean(buf, size + 1);
		if (!eof_ok || !feof(stdin) || ferror(stdin))
			fprintf(stderr,
			    "pwqcheck: Error reading standard input.\n");
		return NULL;
	}

	if (strlen(buf) >= size) {
		clean(buf, size + 1);
		fprintf(stderr, "pwqcheck: Line too long.\n");
		return NULL;
	}

	if ((p = strpbrk(buf, "\r\n")))
		*p = '\0';

	return buf;
}

static char *extract_string(char **stringp)
{
	char *token = *stringp, *colon;

	if (!token)
		return "";

	colon = strchr(token, ':');
	if (colon) {
		*colon = '\0';
		*stringp = colon + 1;
	} else
		*stringp = NULL;

	return token;
}

static struct passwd *parse_pwline(char *line, struct passwd *pw)
{
	if (!strchr(line, ':')) {
		struct passwd *p = getpwnam(line);
		endpwent();
		if (!p) {
			fprintf(stderr, "pwqcheck: User not found.\n");
			return NULL;
		}
		if (p->pw_passwd)
			memset(p->pw_passwd, 0, strlen(p->pw_passwd));
		memcpy(pw, p, sizeof(*pw));
	} else {
		memset(pw, 0, sizeof(*pw));
		pw->pw_name = extract_string(&line);
		pw->pw_passwd = extract_string(&line);
		extract_string(&line); /* uid */
		extract_string(&line); /* gid */
		pw->pw_gecos = extract_string(&line);
		pw->pw_dir = extract_string(&line);
		pw->pw_shell = line ? line : "";
		if (!*pw->pw_name || !*pw->pw_dir) {
			fprintf(stderr, "pwqcheck: Invalid passwd entry.\n");
			return NULL;
		}
	}
	return pw;
}

static void
print_help(void)
{
	puts("Check passphrase quality.\n"
	    "\nFor each passphrase to check, pwqcheck reads up to 3 lines from standard input:\n"
	    "  first line is a new passphrase,\n"
	    "  second line is an old passphrase, and\n"
	    "  third line is either an existing account name or a passwd entry.\n"
	    "\nUsage: pwqcheck [options]\n"
	    "\nValid options are:\n"
	    "  min=N0,N1,N2,N3,N4\n"
	    "       set minimum allowed lengths for different kinds of passphrases;\n"
	    "  max=N\n"
	    "       set maximum allowed passphrase length;\n"
	    "  passphrase=N\n"
	    "       set number of words required for a passphrase;\n"
	    "  match=N\n"
	    "       set length of common substring in substring check;\n"
	    "  config=FILE\n"
	    "       load config FILE in passwdqc.conf format;\n"
	    "  -1\n"
	    "       read just 1 line (new passphrase);\n"
	    "  -2\n"
	    "       read just 2 lines (new and old passphrases);\n"
	    "  --multi\n"
	    "       check multiple passphrases (until EOF);\n"
	    "  --version\n"
	    "       print program version and exit;\n"
	    "  -h or --help\n"
	    "       print this help text and exit.");
}

int main(int argc, const char **argv)
{
	passwdqc_params_t params;
	const char *check_reason;
	char *parse_reason, *newpass, *oldpass, *pwline;
	struct passwd pwbuf, *pw;
	int lines_to_read = 3, multi = 0;
	int size = 8192;
	int rc = 1;

	while (argc > 1 && argv[1][0] == '-') {
		const char *arg = argv[1];

		if (!strcmp("-h", arg) || !strcmp("--help", arg)) {
			print_help();
			return 0;
		}

		if (!strcmp("--version", arg)) {
			printf("pwqcheck version %s\n", PASSWDQC_VERSION);
			return 0;
		}

		if ((arg[1] == '1' || arg[1] == '2') && !arg[2]) {
			lines_to_read = arg[1] - '0';
			goto next_arg;
		}

		if (!strcmp("--multi", arg)) {
			multi = 1;
			goto next_arg;
		}

		break;

next_arg:
		argc--; argv++;
	}

	passwdqc_params_reset(&params);
	if (argc > 1 &&
	    passwdqc_params_parse(&params, &parse_reason, argc - 1,
		argv + 1)) {
		fprintf(stderr, "pwqcheck: %s\n",
		    (parse_reason ? parse_reason : "Out of memory"));
		free(parse_reason);
		return rc;
	}

	if (params.qc.max + 1 > size)
		size = params.qc.max + 1;

next_pass:
	oldpass = pwline = NULL; pw = NULL;
	if (!(newpass = read_line(size, multi))) {
		if (multi && feof(stdin) && !ferror(stdin) &&
		    fflush(stdout) >= 0)
			rc = 0;
		goto done;
	}
	if (lines_to_read >= 2 && !(oldpass = read_line(size, 0)))
		goto done;
	if (lines_to_read >= 3 && (!(pwline = read_line(size, 0)) ||
	    !parse_pwline(pwline, pw = &pwbuf)))
		goto done;

	check_reason = passwdqc_check(&params.qc, newpass, oldpass, pw);
	if (!check_reason) {
		if (multi)
			printf("OK: %s\n", newpass);
		else if (puts("OK") >= 0 && fflush(stdout) >= 0)
			rc = 0;
		goto cleanup;
	}
	if (multi)
		printf("Bad passphrase (%s): %s\n", check_reason, newpass);
	else
		printf("Bad passphrase (%s)\n", check_reason);

cleanup:
	memset(&pwbuf, 0, sizeof(pwbuf));
	clean(pwline, size);
	clean(oldpass, size);
	clean(newpass, size);

	if (multi)
		goto next_pass;

	return rc;

done:
	multi = 0;
	goto cleanup;
}
