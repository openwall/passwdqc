#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "passwdqc.h"

static char *read_line(int size)
{
	char *p, *buf = malloc(size);

	if (!buf) {
		fprintf(stderr, "pwqcheck: Memory allocation failed.\n");
		return NULL;
	}

	if (!fgets(buf, size, stdin)) {
		free(buf);
		fprintf(stderr, "pwqcheck: Error reading standard input.\n");
		return NULL;
	}

	if ((p = strpbrk(buf, "\r\n")))
		*p = '\0';

	return buf;
}

static char *extract_string(char **stringp)
{
	char *p = strsep(stringp, ":");

	return p ? p : "";
}

static int extract_int(char **stringp)
{
	char *p = strsep(stringp, ":");

	return p ? atoi(p) : 0;
}

static struct passwd *parse_pwbuf(char *buf, struct passwd *pw)
{
	if (!strchr(buf, ':')) {
		struct passwd *p = getpwnam(buf);

		if (!p) {
			fprintf(stderr, "pwqcheck: User not found.\n");
			return NULL;
		}
		memcpy(pw, p, sizeof(*pw));
	} else {
		memset(pw, 0, sizeof(*pw));
		pw->pw_name = extract_string(&buf);
		pw->pw_passwd = extract_string(&buf);
		pw->pw_uid = extract_int(&buf);
		pw->pw_gid = extract_int(&buf);
		pw->pw_gecos = extract_string(&buf);
		pw->pw_dir = extract_string(&buf);
		pw->pw_shell = buf ? buf : "";
		if (!*pw->pw_name || !*pw->pw_dir) {
			fprintf(stderr, "pwqcheck: Invalid passwd entry.\n");
			return NULL;
		}
	}
	return pw;
}

static void clean(char *dst, int size)
{
	if (!dst)
		return;
	memset(dst, 0, size);
	free(dst);
}

int main(int argc, const char **argv)
{
	passwdqc_params_t params;
	const char *check_reason;
	char *parse_reason, *newpass = NULL, *oldpass = NULL, *pwbuf = NULL;
	struct passwd pw;
	int size = 8192;
	int rc = 1;

	if (argc > 1 && argv[1][0] == '-') {
		if (!strcmp("-h", argv[1]) || !strcmp("--help", argv[1])) {
			printf("Check password quality.\n"
			    "\npwqcheck reads 3 lines from standard input:\n"
			    "  first line is new password,\n"
			    "  second line is old password, and\n"
			    "  third line is either existing account name or passwd entry.\n"
			    "\nUsage: pwqcheck [options]\n"
			    "\nValid options are:\n"
			    "  min=N0,N1,N2,N3,N4\n"
			    "       set minimum allowed password lengths for different kinds of passwords;\n"
			    "  max=N\n"
			    "       set maximum allowed password length;\n"
			    "  passphrase=N\n"
			    "       set number of words required for a passphrase;\n"
			    "  match=N\n"
			    "       set length of common substring in substring check;\n"
			    "  --version\n"
			    "       print program version and exit;\n"
			    "  -h or --help\n"
			    "       print this help text and exit.\n");
			return 0;
		}

		if (!strcmp("--version", argv[1])) {
			printf("pwqcheck version %s\n", PASSWDQC_VERSION);
			exit(EXIT_SUCCESS);
		}
	}

	passwdqc_params_reset(&params);
	if (argc > 1 &&
	    passwdqc_params_parse(&params, &parse_reason, argc - 1,
		argv + 1)) {
		fprintf(stderr, "pwqcheck: %s\n",
		    (parse_reason ? parse_reason : "Out of memory"));
		free(parse_reason);
		return 1;
	}

	if (params.qc.max + 1 > size)
		size = params.qc.max + 1;

	if (!(newpass = read_line(size)) || !(oldpass = read_line(size))
	    || !(pwbuf = read_line(size)) || !parse_pwbuf(pwbuf, &pw))
		goto done;

	check_reason = passwdqc_check(&params.qc, newpass, oldpass, &pw);
	if (check_reason) {
		fprintf(stderr, "pwqcheck: Weak password: %s\n", check_reason);
		goto done;
	}

	rc = 0;

      done:
	clean(pwbuf, size);
	clean(oldpass, size);
	clean(newpass, size);

	return rc;
}
