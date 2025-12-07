/*
 * Copyright (c) 2000-2002,2010,2013,2016,2020,2025 by Solar Designer.  See LICENSE.
 */

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS /* we use fopen(), sprintf(), strncat() */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "passwdqc.h" /* also provides <pwd.h> or equivalent "struct passwd" */
#include "passwdqc_filter.h"
#include "wordset_4k.h"
#include "wordlist.h"

#include "passwdqc_i18n.h"

#define REASON_ERROR \
	_("check failed")

#define REASON_SAME \
	_("is the same as the old one")
#define REASON_SIMILAR \
	_("is based on the old one")

#define REASON_SHORT \
	_("too short")
#define REASON_LONG \
	_("too long")

#define REASON_SIMPLESHORT \
	_("not enough different characters or classes for this length")
#define REASON_SIMPLE \
	_("not enough different characters or classes")

#define REASON_PERSONAL \
	_("based on personal login information")

#define REASON_WORD \
	_("based on a dictionary word and not a passphrase")

#define REASON_SEQ \
	_("based on a common sequence of characters and not a passphrase")

#define REASON_WORDLIST \
	_("based on a word list entry")

#define REASON_DENYLIST \
	_("is in deny list")

#define REASON_FILTER \
	_("appears to be in a database")

#define FIXED_BITS			15

typedef unsigned long fixed;

/*
 * Calculates the expected number of different characters for a random
 * password of a given length.  The result is rounded down.  We use this
 * with the _requested_ minimum length (so longer passwords don't have
 * to meet this strict requirement for their length).
 */
static int expected_different(int charset, int length)
{
	fixed x, y, z;

	x = ((fixed)(charset - 1) << FIXED_BITS) / charset;
	y = x;
	while (--length > 0)
		y = (y * x) >> FIXED_BITS;
	z = (fixed)charset * (((fixed)1 << FIXED_BITS) - y);

	return (int)(z >> FIXED_BITS);
}

/*
 * A password is too simple if it is too short for its class, or doesn't
 * contain enough different characters for its class, or doesn't contain
 * enough words for a passphrase.
 *
 * The biases are added to the length, and they may be positive or negative.
 * The passphrase length check uses passphrase_bias instead of bias so that
 * zero may be passed for this parameter when the (other) bias is non-zero
 * because of a dictionary word, which is perfectly normal for a passphrase.
 * The biases do not affect the number of different characters, character
 * classes, and word count.
 */
static int is_simple(const passwdqc_params_qc_t *params,
    const char *newpass, int bias, int passphrase_bias)
{
	int length, classes, words, chars;
	int digits, lowers, uppers, others, unknowns;
	int c, p;

	length = classes = words = chars = 0;
	digits = lowers = uppers = others = unknowns = 0;
	p = ' ';
	while ((c = (unsigned char)newpass[length])) {
		length++;

		if (!isascii(c))
			unknowns++;
		else if (isdigit(c))
			digits++;
		else if (islower(c))
			lowers++;
		else if (isupper(c))
			uppers++;
		else
			others++;

/* A word starts when a letter follows a non-letter or when a non-ASCII
 * character follows a space character.  We treat all non-ASCII characters
 * as non-spaces, which is not entirely correct (there's the non-breaking
 * space character at 0xa0, 0x9a, or 0xff), but it should not hurt. */
		if (isascii(p)) {
			if (isascii(c)) {
				if (isalpha(c) && !isalpha(p))
					words++;
			} else if (isspace(p))
				words++;
		}
		p = c;

/* Count this character just once: when we're not going to see it anymore */
		if (!strchr(&newpass[length], c))
			chars++;
	}

	if (!length)
		return 1;

/* Upper case characters and digits used in common ways don't increase the
 * strength of a password */
	c = (unsigned char)newpass[0];
	if (uppers && isascii(c) && isupper(c))
		uppers--;
	c = (unsigned char)newpass[length - 1];
	if (digits && isascii(c) && isdigit(c))
		digits--;

/* Count the number of different character classes we've seen.  We assume
 * that there are no non-ASCII characters for digits. */
	classes = 0;
	if (digits)
		classes++;
	if (lowers)
		classes++;
	if (uppers)
		classes++;
	if (others)
		classes++;
	if (unknowns && classes <= 1 && (!classes || digits || words >= 2))
		classes++;

	for (; classes > 0; classes--)
	switch (classes) {
	case 1:
		if (length + bias >= params->min[0] &&
		    chars >= expected_different(10, params->min[0]) - 1)
			return 0;
		return 1;

	case 2:
		if (length + bias >= params->min[1] &&
		    chars >= expected_different(36, params->min[1]) - 1)
			return 0;
		if (!params->passphrase_words ||
		    words < params->passphrase_words)
			continue;
		if (length + passphrase_bias >= params->min[2] &&
		    chars >= expected_different(27, params->min[2]) - 1)
			return 0;
		continue;

	case 3:
		if (length + bias >= params->min[3] &&
		    chars >= expected_different(62, params->min[3]) - 1)
			return 0;
		continue;

	case 4:
		if (length + bias >= params->min[4] &&
		    chars >= expected_different(95, params->min[4]) - 1)
			return 0;
		continue;
	}

	return 1;
}

static char *unify(char *dst, const char *src)
{
	const char *sptr;
	char *dptr;
	int c;

	if (!dst && !(dst = malloc(strlen(src) + 1)))
		return NULL;

	sptr = src;
	dptr = dst;
	do {
		c = (unsigned char)*sptr;
		if (isascii(c) && isupper(c))
			c = tolower(c);
		switch (c) {
		case 'a': case '@':
			c = '4'; break;
		case 'e':
			c = '3'; break;
/* Unfortunately, if we translate both 'i' and 'l' to '1', this would
 * associate these two letters with each other - e.g., "mile" would
 * match "MLLE", which is undesired.  To solve this, we'd need to test
 * different translations separately, which is not implemented yet. */
		case 'i':
			c = '1'; break;
		case 'o':
			c = '0'; break;
		case 's': case '$':
			c = '5'; break;
		}
		*dptr++ = c;
	} while (*sptr++);

	return dst;
}

static char *reverse(const char *src)
{
	const char *sptr;
	char *dst, *dptr;

	if (!(dst = malloc(strlen(src) + 1)))
		return NULL;

	sptr = &src[strlen(src)];
	dptr = dst;
	while (sptr > src)
		*dptr++ = *--sptr;
	*dptr = '\0';

	return dst;
}

static void clean(char *dst)
{
	if (!dst)
		return;
	_passwdqc_memzero(dst, strlen(dst));
	free(dst);
}

static int is_word_by_length(const char *s, int n)
{
	while (n && isalpha((int)(unsigned char)*s++))
		n--;
	return !n;
}

#define F_MODE 0xff  /* mode flags mask */
#define F_RM   0     /* remove & credit */
#define F_WORD 1     /* discount word */
#define F_SEQ  2     /* discount sequence */
#define F_REV  0x100 /* needle is reversed */

/*
 * Needle is based on haystack if both contain a long enough common
 * substring and needle would be too simple for a password with the
 * substring either removed with partial length credit for it added
 * or partially discounted for the purpose of the length check.
 */
static int is_based(const passwdqc_params_qc_t *params,
    const char *haystack, const char *haystack_original,
    char *needle, const char *needle_original, unsigned int flags)
{
	char *scratch;
	int length, haystack_length, potential_match_length, potential_match_start;
	int i, j;
	const char *p;
	int worst_bias, worst_passphrase_bias;

	if (!params->match_length)	/* disabled */
		return 0;

	if (params->match_length < 0)	/* misconfigured */
		return 1;

	{
		unsigned char haystack_map[0x100] = {0};
		for (p = haystack, haystack_length = 0; *p; p++)
			haystack_map[(unsigned char)*p] = ++haystack_length;

		if (haystack_length > 0xff) { /* map element overflow */
			potential_match_length = haystack_length;
			potential_match_start = 0;
		} else {
			potential_match_length = 0;
			potential_match_start = -1;
			for (p = needle, i = 0; *p; p++) {
				if (haystack_map[(unsigned char)*p] > i) {
					if (potential_match_start < 0)
						potential_match_start = i;
					if (++i >= haystack_length)
						break;
				} else {
					if (i > potential_match_length)
						potential_match_length = i;
					i = haystack_map[(unsigned char)*p];
				}
			}
			if (i > potential_match_length)
				potential_match_length = i;
		}
	}
	if (potential_match_length < params->match_length)
		return 0;

	length = (int)strlen(needle);

	if ((flags & F_MODE) != F_RM) { /* discount */
		worst_bias = (int)params->match_length - 1 - potential_match_length;
		for (i = 0; i < 5; i++) {
			if (length >= params->min[i] &&
			    length + worst_bias < params->min[i]) /* matters */
				break;
		}
		if (i == 5) /* wouldn't matter for any class */
			return 0;
	}

	scratch = NULL;
	worst_bias = worst_passphrase_bias = 0;

	for (i = potential_match_start; i <= length - params->match_length; i++)
	for (j = params->match_length; j <= potential_match_length && i + j <= length; j++) {
		int bias = 0;
		char save = needle[i + j];
		needle[i + j] = 0;
		for (p = haystack; (p = strstr(p, &needle[i])); p++) {
			int pos = (flags & F_REV) /* reversed */ ? length - (i + j) : i;
			if ((flags & F_MODE) == F_RM) { /* remove & credit */
				if (!scratch) {
					if (!(scratch = malloc(length + 1)))
						return 1;
				}
				/* remove j chars */
				memcpy(scratch, needle_original, pos);
				memcpy(&scratch[pos], &needle_original[pos + j], length + 1 - (pos + j));
				/* add credit for match_length - 1 chars */
				bias = params->match_length - 1;
				if (is_simple(params, scratch, bias, bias)) {
					clean(scratch);
					return 1;
				}
				break;
			} else { /* discount */
				int passphrase_bias = 0, invariant = 1;
				/* discount j - (match_length - 1) chars */
				bias = (int)params->match_length - 1 - j;
				if ((flags & F_MODE) == F_WORD) { /* words */
					if (!is_word_by_length(&needle_original[pos], j)) {
/* Require a 1 character longer match for substrings containing leetspeak */
						if (!++bias) {
/* The zero bias optimization further below would be wrong, so skip it */
							bias--;
							break;
						}
/* Do discount non-words from passphrases */
						if (length >= params->min[2] && params->passphrase_words && /* optimization */
						    (length - j < (params->passphrase_words - 1) * 2 ||
						    !is_word_by_length(haystack_original + (p - haystack), j)))
							passphrase_bias = bias;
						invariant = passphrase_bias;
					}
				} else {
					passphrase_bias = bias;
				}
				/* bias <= -1 */
				if (bias < worst_bias || passphrase_bias < worst_passphrase_bias) {
					if (is_simple(params, needle_original, bias, passphrase_bias))
						return 1;
					if (bias < worst_bias)
						worst_bias = bias;
					if (passphrase_bias < worst_passphrase_bias)
						worst_passphrase_bias = passphrase_bias;
				}
				if (invariant) /* optimization */
					break;
			}
		}
		needle[i + j] = save;
/* Zero bias implies that there were no matches for this length.  If so,
 * there's no reason to try the next substring length (it would result in
 * no matches as well).  We break out of the substring length loop and
 * proceed with all substring lengths for the next position in needle. */
		if (!bias)
			break;
	}

	clean(scratch);

	return 0;
}

#define READ_LINE_MAX 8192
#define READ_LINE_SIZE (READ_LINE_MAX + 2)

static char *read_line(FILE *f, char *buf)
{
	buf[READ_LINE_MAX] = '\n';

	if (!fgets(buf, READ_LINE_SIZE, f))
		return NULL;

	if (buf[READ_LINE_MAX] != '\n') {
		int c;
		do {
			c = getc(f);
		} while (c != EOF && c != '\n');
		if (ferror(f))
			return NULL;
	}

	char *p;
	if ((p = strpbrk(buf, "\r\n")))
		*p = '\0';

	return buf;
}

/*
 * Common sequences of characters, variations on the word "password", months.
 * We don't need to list any of the entire strings in reverse order because the
 * code checks the new password in both "unified" and "unified and reversed"
 * form against these strings (unifying them first indeed).  We also don't have
 * to include common repeats of characters (e.g., "777", "!!!", "1000") because
 * these are often taken care of by the requirement on the number of different
 * characters.
 */
const char * const seq[] = {
	"abcd1234abc1234567890-=",
	"0123qweasd123abcdefghijklmnopqrstuvwxyz123",
	"0a1b2c3d4e5f6g7h8i9j0",
	"1a2b3c4d5e6f7g8h9i0j",
	"zaqwertyuiop[]\\",
	"qasdfghjkl;'",
	"@123qazxcvbnm,./",
	"1qaz2wsx3edc4rfv5tgb6yhn7ujm8ik,9ol.0p;/-['=]\\",
	"qazwsxedcrfvtgbyhnujmikolp",
	"1q2w3e4r5t6y7u8i9o0p-[=]",
	"qwe123q1w2e3r4t5y6u7i8o9p0[-]=\\",
	"12qw34er56ty78ui90op-=[]",
	"zaq1xsw2cde3vfr4bgt5nhy6mju7,ki8.lo9/;p0",
	"1qa2ws3ed4rf5tg6yh7uj8ik9ol0p;",
	"1qaz1qaz",
	"zaq!1qaz",
	"zaq!2wsx",
	"1password1234567890", /* these must be after the sequences */
	"123pass1234567890",
	"January", /* shorter month names are in wordset_4k */
	"February",
	"September",
	"October",
	"November",
	"December"
};

/*
 * This wordlist check is now the least important given the checks above
 * and the support for passphrases (which are based on dictionary words,
 * and checked by other means).  It is still useful to trap simple short
 * passwords (if short passwords are allowed) that are word-based, but
 * passed the other checks due to uncommon capitalization, digits, and
 * special characters.  We (mis)use the same set of words that are used
 * to generate random passwords.  This list is much smaller than those
 * used for password crackers, and it doesn't contain common passwords
 * that aren't short English words.  We compensate by also having the
 * list of common sequences above, and a tiny built-in list of common
 * passwords.  We also support optional external wordlist (for inexact
 * matching) and deny list (for exact matching).
 */
static const char *is_word_based(const passwdqc_params_qc_t *params,
    char *unified, char *reversed, const char *original)
{
	const char *reason = REASON_ERROR;
#if WORDLIST_LENGTH_MAX > WORDSET_4K_LENGTH_MAX
	char word[WORDLIST_LENGTH_MAX + 1];
	char word_unified[WORDLIST_LENGTH_MAX + 1];
#else
	char word[WORDSET_4K_LENGTH_MAX + 1];
	char word_unified[WORDSET_4K_LENGTH_MAX + 1];
#endif
	char *buf = NULL;
	FILE *f = NULL;
	unsigned int i;

	word[WORDSET_4K_LENGTH_MAX] = '\0';
	if (params->match_length)
	for (i = 0; _passwdqc_wordset_4k[i][0]; i++) {
		memcpy(word, _passwdqc_wordset_4k[i], WORDSET_4K_LENGTH_MAX);
		int length = (int)strlen(word);
		if (length < params->match_length)
			continue;
		if (!memcmp(word, _passwdqc_wordset_4k[i + 1], length))
			continue;
		unify(word_unified, word);
		if (is_based(params, word_unified, word, unified, original, F_WORD) ||
		    is_based(params, word_unified, word, reversed, original, F_WORD|F_REV)) {
			reason = REASON_WORD;
			goto out;
		}
	}

	if (params->match_length)
	for (i = 0; i < sizeof(seq) / sizeof(seq[0]); i++) {
		unsigned int flags = (seq[i][0] >= 'A' && seq[i][0] <= 'Z') ? F_WORD : F_SEQ;
		char *seq_i = unify(NULL, seq[i]);
		if (!seq_i)
			goto out;
		if (is_based(params, seq_i, seq[i], unified, original, flags) ||
		    is_based(params, seq_i, seq[i], reversed, original, flags | F_REV)) {
			clean(seq_i);
			reason = (flags == F_WORD) ? REASON_WORD : REASON_SEQ;
			goto out;
		}
		clean(seq_i);
	}

	if (params->match_length && params->match_length <= 4)
	for (i = 1900; i <= 2039; i++) {
		sprintf(word, "%u", i);
		if (is_based(params, word, word, unified, original, F_SEQ) ||
		    is_based(params, word, word, reversed, original, F_SEQ|F_REV)) {
			reason = REASON_SEQ;
			goto out;
		}
	}

	if (params->match_length && !params->wordlist) {
		const char *p = _passwdqc_wordlist;
		char *q = word;
		unsigned int reuse = 1;
		do {
			if (--reuse > q - word) /* shouldn't happen */
				reuse = 0;
			q = word + reuse;
			while ((unsigned char)*p >= WORDLIST_LENGTH_MAX && q < word + WORDLIST_LENGTH_MAX)
				*q++ = *p++;
			*q = 0;
			if (q - word < params->match_length)
				continue;
			unify(word_unified, word);
			if (is_based(params, word_unified, word, unified, original, F_WORD) ||
			    is_based(params, word_unified, word, reversed, original, F_WORD|F_REV))
				goto out_wordlist;
		} while ((reuse = *p++));
	}

	if (params->wordlist || params->denylist)
		if (!(buf = malloc(READ_LINE_SIZE * 2)))
			goto out;

	if (params->wordlist) {
		if (!(f = fopen(params->wordlist, "r")))
			goto out;
		while (read_line(f, buf)) {
			char *buf_unified = buf + READ_LINE_SIZE;
			unify(buf_unified, buf);
			if (!strcmp(buf_unified, unified) || !strcmp(buf_unified, reversed))
				goto out_wordlist;
			if (!params->match_length ||
			    strlen(buf_unified) < (size_t)params->match_length)
				continue;
			if (is_based(params, buf_unified, buf, unified, original, F_WORD) ||
			    is_based(params, buf_unified, buf, reversed, original, F_WORD|F_REV)) {
out_wordlist:
				reason = REASON_WORDLIST;
				goto out;
			}
		}
		if (ferror(f))
			goto out;
		fclose(f); f = NULL;
	}

	if (params->denylist) {
		if (!(f = fopen(params->denylist, "r")))
			goto out;
		while (read_line(f, buf)) {
			if (!strcmp(buf, original)) {
				reason = REASON_DENYLIST;
				goto out;
			}
		}
		if (ferror(f))
			goto out;
	}

	reason = NULL;

out:
	if (f)
		fclose(f);
	if (buf) {
		_passwdqc_memzero(buf, READ_LINE_SIZE * 2);
		free(buf);
	}
	_passwdqc_memzero(word, sizeof(word));
	return reason;
}

const char *passwdqc_check(const passwdqc_params_qc_t *params,
    const char *newpass, const char *oldpass, const struct passwd *pw)
{
	char truncated[9];
	char *u_newpass = NULL, *u_reversed = NULL;
	char *u_oldpass = NULL;
	char *u_name = NULL, *u_gecos = NULL, *u_dir = NULL;
	const char *reason = REASON_ERROR;

	size_t length = strlen(newpass);

	if (length < (size_t)params->min[4]) {
		reason = REASON_SHORT;
		goto out;
	}

	if (length > 10000) {
		reason = REASON_LONG;
		goto out;
	}

	if (length > (size_t)params->max) {
		if (params->max == 8) {
			truncated[0] = '\0';
			strncat(truncated, newpass, 8);
			newpass = truncated;
			length = 8;
			if (oldpass && !strncmp(oldpass, newpass, 8)) {
				reason = REASON_SAME;
				goto out;
			}
		} else {
			reason = REASON_LONG;
			goto out;
		}
	}

	if (oldpass && !strcmp(oldpass, newpass)) {
		reason = REASON_SAME;
		goto out;
	}

	if (is_simple(params, newpass, 0, 0)) {
		reason = REASON_SIMPLE;
		if (length < (size_t)params->min[1] &&
		    params->min[1] <= params->max)
			reason = REASON_SIMPLESHORT;
		goto out;
	}

	if (!(u_newpass = unify(NULL, newpass)))
		goto out; /* REASON_ERROR */
	if (!(u_reversed = reverse(u_newpass)))
		goto out;
	if (oldpass && !(u_oldpass = unify(NULL, oldpass)))
		goto out;
	if (pw) {
		if (!(u_name = unify(NULL, pw->pw_name)) ||
		    !(u_gecos = unify(NULL, pw->pw_gecos)) ||
		    !(u_dir = unify(NULL, pw->pw_dir)))
			goto out;
	}

	if (oldpass && params->similar_deny &&
	    (is_based(params, u_oldpass, NULL, u_newpass, newpass, F_RM) ||
	     is_based(params, u_oldpass, NULL, u_reversed, newpass, F_RM|F_REV))) {
		reason = REASON_SIMILAR;
		goto out;
	}

	if (pw &&
	    (is_based(params, u_name, NULL, u_newpass, newpass, F_RM) ||
	     is_based(params, u_name, NULL, u_reversed, newpass, F_RM|F_REV) ||
	     is_based(params, u_gecos, NULL, u_newpass, newpass, F_RM) ||
	     is_based(params, u_gecos, NULL, u_reversed, newpass, F_RM|F_REV) ||
	     is_based(params, u_dir, NULL, u_newpass, newpass, F_RM) ||
	     is_based(params, u_dir, NULL, u_reversed, newpass, F_RM|F_REV))) {
		reason = REASON_PERSONAL;
		goto out;
	}

	reason = is_word_based(params, u_newpass, u_reversed, newpass);

	if (!reason && params->filter) {
		passwdqc_filter_t flt;
		reason = REASON_ERROR;
		if (passwdqc_filter_open(&flt, params->filter))
			goto out;
		int result = passwdqc_filter_lookup(&flt, newpass);
		passwdqc_filter_close(&flt);
		if (result < 0)
			goto out;
		reason = result ? REASON_FILTER : NULL;
	}

out:
	_passwdqc_memzero(truncated, sizeof(truncated));
	clean(u_newpass);
	clean(u_reversed);
	clean(u_oldpass);
	clean(u_name);
	clean(u_gecos);
	clean(u_dir);

	return reason;
}
