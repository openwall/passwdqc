/*
 * Copyright (c) 2000-2002,2005,2008,2010,2013,2016 by Solar Designer
 * See LICENSE
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include "passwdqc.h"
#include "wordset_4k.h"

/*
 * We separate words in the generated "passphrases" with random special
 * characters out of a set of 16 (so we encode 4 bits per separator
 * character).  To enable the use of our "passphrases" within FTP URLs
 * (and similar), we pick characters that are defined by RFC 3986 as
 * being safe within "userinfo" part of URLs without encoding and
 * without having a special meaning.  Out of those, we avoid characters
 * that are visually ambiguous or difficult over the phone.  This
 * happens to leave us with exactly 8 symbols, and we add 8 digits that
 * are not visually ambiguous.  Unfortunately, the exclamation mark
 * might be confused for the digit 1 (which we don't use), though.
 */
#define SEPARATORS			"-_!$&*+=23456789"

/*
 * Number of bits encoded per separator character.
 */
#define SEPARATOR_BITS			4

/*
 * Number of bits encoded per word.  We use 4096 words, which gives 12 bits,
 * and we toggle the case of the first character, which gives one bit more.
 */
#define WORD_BITS			13

/*
 * Number of bits encoded per separator and word.
 */
#define SWORD_BITS \
	(SEPARATOR_BITS + WORD_BITS)

/*
 * Maximum number of words to use.
 */
#define WORDS_MAX			8

/*
 * Minimum and maximum number of bits to encode.  With the settings above,
 * these are 24 and 136, respectively.
 */
#define BITS_MIN \
	(2 * (WORD_BITS - 1))
#define BITS_MAX \
	(WORDS_MAX * SWORD_BITS)

static int read_loop(int fd, unsigned char *buffer, int count)
{
	int offset, block;

	offset = 0;
	while (count > 0) {
		block = read(fd, &buffer[offset], count);

		if (block < 0) {
			if (errno == EINTR)
				continue;
			return block;
		}
		if (!block)
			return offset;

		offset += block;
		count -= block;
	}

	return offset;
}

char *passwdqc_random(const passwdqc_params_qc_t *params)
{
	char output[0x100], *retval;
	int bits;
	int word_count, trailing_separator, use_separators, toggle_case;
	int i;
	unsigned int max_length, length, extra;
	const char *start, *end;
	int fd;
	unsigned char bytes[3];

	bits = params->random_bits;
	if (bits < BITS_MIN || bits > BITS_MAX)
		return NULL;

/*
 * Calculate the number of words to use.  The first word is always present
 * (hence the "1 +" and the "- WORD_BITS").  Each one of the following words,
 * if any, is prefixed by a separator character, so we use SWORD_BITS when
 * calculating how many additional words to use.  We divide "bits - WORD_BITS"
 * by SWORD_BITS with rounding up (hence the addition of "SWORD_BITS - 1").
 */
	word_count = 1 + (bits + (SWORD_BITS - 1 - WORD_BITS)) / SWORD_BITS;

/*
 * Special case: would we still encode enough bits if we omit the final word,
 * but keep the would-be-trailing separator?
 */
	trailing_separator = (SWORD_BITS * (word_count - 1) >= bits);
	word_count -= trailing_separator;

/*
 * To determine whether we need to use different separator characters or maybe
 * not, calculate the number of words we'd need to use if we don't use
 * different separators.  We calculate it by dividing "bits" by WORD_BITS with
 * rounding up (hence the addition of "WORD_BITS - 1").  The resulting number
 * is either the same as or greater than word_count.  Use different separators
 * only if their use, in the word_count calculation above, has helped reduce
 * word_count.
 */
	use_separators = ((bits + (WORD_BITS - 1)) / WORD_BITS != word_count);
	trailing_separator &= use_separators;

/*
 * Toggle case of the first character of each word only if we wouldn't achieve
 * sufficient entropy otherwise.
 */
	toggle_case = (bits >
	    ((WORD_BITS - 1) * word_count) +
	    (use_separators ?
	    (SEPARATOR_BITS * (word_count - !trailing_separator)) : 0));

/*
 * Calculate and check the maximum possible length of a "passphrase" we may
 * generate for a given word_count.  We add 1 to WORDSET_4K_LENGTH_MAX to
 * account for separators (whether different or not).  When there's no
 * trailing separator, we subtract 1.  The check against sizeof(output) uses
 * ">=" to account for NUL termination.
 */
	max_length = word_count * (WORDSET_4K_LENGTH_MAX + 1) -
	    !trailing_separator;
	if (max_length >= sizeof(output) || (int)max_length > params->max)
		return NULL;

	if ((fd = open("/dev/urandom", O_RDONLY)) < 0)
		return NULL;

	retval = NULL;
	length = 0;
	do {
		if (read_loop(fd, bytes, sizeof(bytes)) != sizeof(bytes))
			goto out;

/*
 * Append a word.  Treating bytes as little-endian, we use bits 0 to 11 for the
 * word index, and bit 13 for toggling the case of the first character.  Bits
 * 12, 14, and 15 are left unused.  Bits 16 to 23 are left for the separator.
 */
		i = (((int)bytes[1] & 0x0f) << 8) | (int)bytes[0];
		start = _passwdqc_wordset_4k[i];
		end = memchr(start, '\0', WORDSET_4K_LENGTH_MAX);
		if (!end)
			end = start + WORDSET_4K_LENGTH_MAX;
		extra = end - start;
/* The ">=" leaves room for either one more separator or NUL */
		if (length + extra >= sizeof(output))
			goto out;
		memcpy(&output[length], start, extra);
		if (toggle_case) {
/* Toggle case if bit set (we assume ASCII) */
			output[length] ^= bytes[1] & 0x20;
			bits--;
		}
		length += extra;
		bits -= WORD_BITS - 1;

		if (bits <= 0)
			break;

/*
 * Append a separator character.  We use bits 16 to 19.  Bits 20 to 23 are left
 * unused.
 *
 * Special case: we may happen to leave a trailing separator if it provides
 * enough bits on its own.  With WORD_BITS 13 and SEPARATOR_BITS 4, this
 * happens e.g. for bits values from 31 to 34, 48 to 51, 65 to 68.
 */
		if (use_separators) {
			i = bytes[2] & 0x0f;
			output[length++] = SEPARATORS[i];
			bits -= SEPARATOR_BITS;
		} else
			output[length++] = SEPARATORS[0];
	} while (bits > 0);

/*
 * Since we may have added a separator after the check in the loop above, we
 * must check again now.
 */
	if (length < sizeof(output)) {
		output[length] = '\0';
		retval = strdup(output);
	}

out:
	_passwdqc_memzero(bytes, sizeof(bytes));
	_passwdqc_memzero(output, length);

	close(fd);

	return retval;
}
