/*
 * Copyright (c) 2000-2002,2005,2008,2010,2013 by Solar Designer.  See LICENSE.
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
#define WORDS_MAX			7

/*
 * Minimum and maximum number of bits to encode.  With the settings above,
 * these are 26 and 132, respectively.
 */
#define BITS_MIN \
	(2 * WORD_BITS)
#define BITS_MAX \
	(WORD_BITS + WORDS_MAX * (SEPARATOR_BITS + WORD_BITS))

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
	int word_count, use_separators, i;
	unsigned int length, extra;
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
 * To determine whether we need to use different separator characters or maybe
 * not, calculate the number of words we'd need to use if we don't use
 * different separators.  We calculate it by dividing "bits" by WORD_BITS with
 * rounding up (hence the addition of "WORD_BITS - 1").  The resulting number
 * is either the same as or greater than word_count.  Use different separators
 * only if their use, in the word_count calculation above, has helped reduce
 * word_count.
 */
	use_separators = ((bits + (WORD_BITS - 1)) / WORD_BITS != word_count);

/*
 * Calculate and check the maximum possible length of a "passphrase" we may
 * generate for a given word_count.  We add 1 to WORDSET_4K_LENGTH_MAX to
 * account for separators (whether different or not).  We subtract 1 because
 * the number of one-character separators is one less than the number of words.
 * The check against sizeof(output) uses ">=" to account for NUL termination.
 */
	length = word_count * (WORDSET_4K_LENGTH_MAX + 1) - 1;
	if (length >= sizeof(output) || (int)length > params->max)
		return NULL;

	if ((fd = open("/dev/urandom", O_RDONLY)) < 0)
		return NULL;

	length = 0;
	do {
		if (read_loop(fd, bytes, sizeof(bytes)) != sizeof(bytes)) {
			close(fd);
			return NULL;
		}

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
		if (length + extra >= sizeof(output) - 1) {
			close(fd);
			return NULL;
		}
		memcpy(&output[length], start, extra);
		output[length] ^= bytes[1] & 0x20; /* toggle case if bit set */
		length += extra;
		bits -= WORD_BITS;

		if (bits <= 0)
			break;

/*
 * Append a separator character.  We use bits 16 to 19.  Bits 20 to 23 are left
 * unused.
 *
 * Special case: we may happen to generate one word less than word_count if the
 * final separator provides enough bits on its own.  This was not accounted for
 * in the calculations prior to this loop, but it is here.  With WORD_BITS 13
 * and SEPARATOR_BITS 4, this happens e.g. for bits values from 66 to 68.
 */
		if (use_separators) {
			i = bytes[2] & 0x0f;
			output[length++] = SEPARATORS[i];
			bits -= SEPARATOR_BITS;
		} else
			output[length++] = SEPARATORS[0];
	} while (bits > 0);

	memset(bytes, 0, sizeof(bytes));

	close(fd);

	output[length] = '\0';
	retval = strdup(output);
	memset(output, 0, length);
	return retval;
}
