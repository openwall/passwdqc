/*
 * Copyright (c) 2000-2003,2005 by Solar Designer
 * Copyright (c) 2008,2009 by Dmitry V. Levin
 * See LICENSE
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "passwdqc.h"
#include "concat.h"

static const char *skip_prefix(const char *sample, const char *prefix)
{
	size_t len = strlen(prefix);

	if (strncmp(sample, prefix, len))
		return NULL;
	return sample + len;
}

static int
parse_option(passwdqc_params_t *params, char **reason, const char *option)
{
	const char *err = "Invalid parameter value";
	const char *p;
	char *e;
	int i, rc = 0;
	unsigned long v;

	*reason = NULL;
	if ((p = skip_prefix(option, "min="))) {
		for (i = 0; i < 5; i++) {
			if (!strncmp(p, "disabled", 8)) {
				v = INT_MAX;
				p += 8;
			} else {
				v = strtoul(p, &e, 10);
				p = e;
			}
			if (i < 4 && *p++ != ',')
				goto parse_error;
			if (v > INT_MAX)
				goto parse_error;
			if (i && (int)v > params->qc.min[i - 1])
				goto parse_error;
			params->qc.min[i] = v;
		}
		if (*p)
			goto parse_error;
	} else if ((p = skip_prefix(option, "max="))) {
		v = strtoul(p, &e, 10);
		if (*e || v < 8 || v > INT_MAX)
			goto parse_error;
		params->qc.max = v;
	} else if ((p = skip_prefix(option, "passphrase="))) {
		v = strtoul(p, &e, 10);
		if (*e || v > INT_MAX)
			goto parse_error;
		params->qc.passphrase_words = v;
	} else if ((p = skip_prefix(option, "match="))) {
		v = strtoul(p, &e, 10);
		if (*e || v > INT_MAX)
			goto parse_error;
		params->qc.match_length = v;
	} else if ((p = skip_prefix(option, "similar="))) {
		if (!strcmp(p, "permit"))
			params->qc.similar_deny = 0;
		else if (!strcmp(p, "deny"))
			params->qc.similar_deny = 1;
		else
			goto parse_error;
	} else if ((p = skip_prefix(option, "random="))) {
		v = strtoul(p, &e, 10);
		if (!strcmp(e, ",only")) {
			e += 5;
			params->qc.min[4] = INT_MAX;
		}
		if (*e || (v && v < 26) || v > 81)
			goto parse_error;
		params->qc.random_bits = v;
	} else if ((p = skip_prefix(option, "enforce="))) {
		params->pam.flags &= ~F_ENFORCE_MASK;
		if (!strcmp(p, "users"))
			params->pam.flags |= F_ENFORCE_USERS;
		else if (!strcmp(p, "everyone"))
			params->pam.flags |= F_ENFORCE_EVERYONE;
		else if (strcmp(p, "none"))
			goto parse_error;
	} else if (!strcmp(option, "non-unix")) {
		if (params->pam.flags & F_CHECK_OLDAUTHTOK)
			goto parse_error;
		params->pam.flags |= F_NON_UNIX;
	} else if ((p = skip_prefix(option, "retry="))) {
		v = strtoul(p, &e, 10);
		if (*e || v > INT_MAX)
			goto parse_error;
		params->pam.retry = v;
	} else if ((p = skip_prefix(option, "ask_oldauthtok"))) {
		params->pam.flags &= ~F_ASK_OLDAUTHTOK_MASK;
		if (params->pam.flags & F_USE_FIRST_PASS)
			goto parse_error;
		if (!p[0])
			params->pam.flags |= F_ASK_OLDAUTHTOK_PRELIM;
		else if (!strcmp(p, "=update"))
			params->pam.flags |= F_ASK_OLDAUTHTOK_UPDATE;
		else
			goto parse_error;
	} else if (!strcmp(option, "check_oldauthtok")) {
		if (params->pam.flags & F_NON_UNIX)
			goto parse_error;
		params->pam.flags |= F_CHECK_OLDAUTHTOK;
	} else if (!strcmp(option, "use_first_pass")) {
		if (params->pam.flags & F_ASK_OLDAUTHTOK_MASK)
			goto parse_error;
		params->pam.flags |= F_USE_FIRST_PASS | F_USE_AUTHTOK;
	} else if (!strcmp(option, "use_authtok")) {
		params->pam.flags |= F_USE_AUTHTOK;
	} else if ((p = skip_prefix(option, "config="))) {
		if ((rc = passwdqc_params_load(params, reason, p)))
			goto parse_error;
	} else {
		err = "Invalid parameter";
		goto parse_error;
	}

	return 0;

parse_error:
	e = concat("Error parsing parameter \"", option, "\": ",
	    (rc ? (*reason ? *reason : "Out of memory") : err), NULL);
	free(*reason);
	*reason = e;
	return rc ? rc : -1;
}

int
passwdqc_params_parse(passwdqc_params_t *params, char **reason,
    int argc, const char *const *argv)
{
	int i;

	*reason = NULL;
	for (i = 0; i < argc; ++i) {
		int rc;

		if ((rc = parse_option(params, reason, argv[i])))
			return rc;
	}
	return 0;
}

static passwdqc_params_t defaults = {
	{
		{INT_MAX, 24, 11, 8, 7},	/* min */
		40,				/* max */
		3,				/* passphrase_words */
		4,				/* match_length */
		1,				/* similar_deny */
		47				/* random_bits */
	},
	{
		F_ENFORCE_EVERYONE,		/* flags */
		3				/* retry */
	}
};

void passwdqc_params_reset(passwdqc_params_t *params)
{
	*params = defaults;
}
