/*
 * Copyright (c) 2020 by Solar Designer
 * See LICENSE
 */

#ifdef _MSC_VER
#define _CRT_NONSTDC_NO_WARNINGS /* we use unlink() */
#define _CRT_SECURE_NO_WARNINGS /* we use fopen() */
#include <io.h>
#else
#include <unistd.h> /* for unlink() */
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "md4.h"
#include "passwdqc.h"
#define PASSWDQC_FILTER_INTERNALS
#include "passwdqc_filter.h"

/* Flags corresponding to command-line options, can use bits 3 to 23 */
#define OPT_VERBOSE			0x08
#define OPT_COUNT			0x10
#define OPT_LINE_NUMBER			0x20
#define OPT_INVERT_MATCH		0x40
#define OPT_PRE_HASHED			0x80

#define OPT_HASH_ID_SHIFT		8
#define OPT_HASH_MD4			(PASSWDQC_FILTER_HASH_MD4 << OPT_HASH_ID_SHIFT)
#define OPT_HASH_NTLM_CP1252		(PASSWDQC_FILTER_HASH_NTLM_CP1252 << OPT_HASH_ID_SHIFT)
#define OPT_HASH_ID_MASK		(OPT_HASH_MD4 | OPT_HASH_NTLM_CP1252)

#define OPT_FP_RATE			0x1000
#define OPT_FP_RATE_AT_HIGH_LOAD	0x2000
#define OPT_TEST_FP_RATE		0x4000

/* Bitmask of all supported hash types */
#define OPT_HASH_ALL (OPT_HASH_MD4 | OPT_HASH_NTLM_CP1252)

/* Bitmask of options only valid in lookup mode */
#define OPT_LOOKUP (OPT_COUNT | OPT_LINE_NUMBER | OPT_INVERT_MATCH)

/* Bitmask of options only valid in create and insert modes */
#define OPT_INSERT (OPT_FP_RATE | OPT_FP_RATE_AT_HIGH_LOAD)

/*
 * Cache line alignment is very important here because of the pattern in which
 * elements of ssencode[] are used.  With 64-byte cache lines, we use 444 of
 * them with proper alignment, but at worst 563 otherwise.  With 128-byte cache
 * lines, we use 260 with proper alignment, 319 with alignment to 64 but not
 * 128 bytes, and at worst 374 otherwise.  (These numbers do not include the
 * additional uses by variables that we insert into the largest gap.)
 */
#ifdef __GNUC__
__attribute__ ((aligned (128)))
#endif
static union {
	uint16_t ssencode[0x10000];
	struct {
/*
 * Skip until the largest gap in ssencode[], which is from 0xf000 to 0xfffe.
 * We skip an additional 0x30 elements (96 bytes) so that the hot part of the
 * header (its second 32 bytes) starts at the beginning of a cache line and
 * further hot fields that we have in here fall into the same cache line.
 * Moreover, with the current fields this lets us have the first 8 bytes of
 * ssdecode[] in the same cache line as well, which makes the rest of it fit
 * into 121 64-byte cache lines (otherwise, with poor luck it'd need 122).
 * This brings our total cache usage for these globals to (444+1+121)*64 =
 * 36224 bytes.
 */
		uint16_t skip[0xf030];
		passwdqc_filter_header_t header;
		uint64_t maxkicks;
		passwdqc_filter_packed_t *packed;
		passwdqc_filter_i_t nbuckets;
		uint32_t options; /* bitmask of OPT_* flags */
		uint16_t ssdecode[3876];
	} s;
} globals;

#define ssencode globals.ssencode
#define header globals.s.header
#define maxkicks globals.s.maxkicks
#define packed globals.s.packed
#define nbuckets globals.s.nbuckets
#define options globals.s.options
#define ssdecode globals.s.ssdecode

/* Store a copy of (updated) header.threshold in the hottest cache line */
#define SET_THRESHOLD(x) options = (options & 0xffffff) | ((uint32_t)(x) << 24);
#define GET_THRESHOLD (options >> 24)

/* For inserts only, also store (updated) header.bucket_size */
#define SET_BUCKET_SIZE(x) options = (options & ~7U) | (x);
#define GET_BUCKET_SIZE (options & 7)

static void ssinit(void)
{
	unsigned int a, b, c, d, n = 0;
	for (d = 0; d < 16; d++)
	for (c = d; c < 16; c++)
	for (b = c; b < 16; b++)
	for (a = b; a < 16; a++) {
		uint16_t ssd = (d << 12) | (c << 8) | (b << 4) | a;
		assert(ssd == passwdqc_filter_ssdecode(n));
		assert(n < sizeof(ssdecode) / sizeof(ssdecode[0]));
		ssdecode[n++] = ssd;
		ssencode[ssd] = n;
	}
	assert(n == sizeof(ssdecode) / sizeof(ssdecode[0]));
	assert(&ssdecode[n] <= &ssencode[0xffff]);
}

static inline unsigned int unpack(passwdqc_filter_unpacked_t *dst, const passwdqc_filter_packed_t *src)
{
	/* -1 cast to unsigned becomes greater than bucket size */
	return (unsigned int)passwdqc_filter_unpack(dst, src, ssdecode);
}

static inline int lookup(passwdqc_filter_hash_t *h, passwdqc_filter_f_t fmask)
{
	passwdqc_filter_i_t i = passwdqc_filter_h2i(h, nbuckets);
	passwdqc_filter_f_t f = passwdqc_filter_h2f(h);

	passwdqc_filter_unpacked_t u;
	unsigned int n = unpack(&u, &packed[i]);
	if (unlikely(n > GET_BUCKET_SIZE))
		return -1;

	unsigned int j;
	for (j = 0; j < n; j++)
		if (passwdqc_filter_f_eq(u.slots[j] & fmask, f & fmask, GET_BUCKET_SIZE))
			return 1;

/*
 * We can skip checking the secondary bucket on lookup when the primary one
 * is below the fill threshold, but only as long as there are no deletes yet.
 * Whenever a delete brings a bucket from at to below header.threshold, it
 * must update header.threshold, and then we must use that in here (we do).
 */
	if (n < GET_THRESHOLD)
		return 0;

	n = unpack(&u, &packed[passwdqc_filter_alti(i, f, nbuckets)]);
	if (unlikely(n > GET_BUCKET_SIZE))
		return -1;

	for (j = 0; j < n; j++)
		if (passwdqc_filter_f_eq(u.slots[j] & fmask, f & fmask, GET_BUCKET_SIZE))
			return 1;

	return 0;
}

/*
 * Code specialization flags assuming pack() will be inlined (the corresponding
 * checks would best be omitted if not inlining).
 */
#define PACK_MASK_OLD 1
#define PACK_MASK_NEW 2
#define PACK_MASK_ALL (PACK_MASK_OLD | PACK_MASK_NEW)

static force_inline void pack(passwdqc_filter_packed_t *dst, const passwdqc_filter_unpacked_t *src, unsigned int n, int flags)
{
	if (n == 4) { /* 4x 33-bit as 12-bit semi-sort index, 4x 29-bit */
/*
 * Encode 4x 33-bit fingerprints as 12-bit semi-sort index of 4x 4-bit values
 * corresponding to most significant bits of each fingerprint, followed by 4x
 * 29-bit values holding the rest of the fingerprint data in original form.
 */
		const unsigned int fbits = 33;
		const passwdqc_filter_f_t fmask = ((passwdqc_filter_f_t)1 << fbits) - 1;
		passwdqc_filter_f_t a = src->slots[0];
		passwdqc_filter_f_t b = src->slots[1];
		passwdqc_filter_f_t c = src->slots[2];
		passwdqc_filter_f_t d = src->slots[3];
		if (flags & PACK_MASK_OLD) {
			a &= fmask; b &= fmask; c &= fmask;
			if (flags & PACK_MASK_NEW)
				d &= fmask;
		}
#define SORT(x, y) if (x < y) { passwdqc_filter_f_t z = x; x = y; y = z; }
		SORT(a, b)
		SORT(c, d)
/*
 * The check for "b < c" can be skipped and further 3 SORT() steps performed
 * unconditionally.  This check is a controversial optimization for the case of
 * updating previously sorted lists.  Unfortunately, it increases the average
 * number of comparisons (but not swaps) for random lists.
 */
		if (b < c) {
			SORT(b, d)
			SORT(a, c)
			SORT(b, c)
		}
		const unsigned int lobits = fbits - 4;
		uint16_t ssd = (uint16_t)(a >> lobits);
		ssd |= (b >> (lobits - 4)) & 0x00f0;
		ssd |= (c >> (lobits - 8)) & 0x0f00;
		ssd |= (d >> (lobits - 12)) & 0xf000;
		const passwdqc_filter_f_t lomask = ((passwdqc_filter_f_t)1 << lobits) - 1;
		a &= lomask;
		b &= lomask;
		c &= lomask;
		d &= lomask;
		dst->lo = a | (b << lobits) | (c << (2 * lobits));
		dst->hi = (c >> (64 - 2 * lobits)) | (d << (3 * lobits - 64)) | ((uint64_t)ssencode[ssd] << (64 - 12));
		return;
	}

	if (n == 3) { /* 11111, 3x 41-bit */
		const unsigned int fbits = 41;
		const passwdqc_filter_f_t fmask = ((passwdqc_filter_f_t)1 << fbits) - 1;
		passwdqc_filter_f_t a = src->slots[0];
		passwdqc_filter_f_t b = src->slots[1];
		passwdqc_filter_f_t c = src->slots[2];
		if (flags & PACK_MASK_OLD) {
			a &= fmask; b &= fmask;
			if (flags & PACK_MASK_NEW)
				c &= fmask;
		}
/*
 * Sorting of fewer than 4 entries is unnecessary, but we use it to detect some
 * kinds of data corruption.  It also very slightly improves compressibility of
 * the resulting filter files.
 */
		SORT(b, c)
		SORT(a, c)
		SORT(a, b)
		dst->lo = a | (b << fbits);
		dst->hi = (b >> (64 - fbits)) | (c << (2 * fbits - 64)) | ((uint64_t)0xf80 << (64 - 12));
		return;
	}

	if (n == 2) { /* 111101, 2x 61-bit */
		const unsigned int fbits = 61;
		const passwdqc_filter_f_t fmask = ((passwdqc_filter_f_t)1 << fbits) - 1;
		passwdqc_filter_f_t a = src->slots[0];
		passwdqc_filter_f_t b = src->slots[1];
		if (flags & PACK_MASK_OLD) {
			a &= fmask;
			if (flags & PACK_MASK_NEW)
				b &= fmask;
		}
		SORT(a, b)
#undef SORT
		dst->lo = a | (b << fbits);
		dst->hi = (b >> (64 - fbits)) | ((uint64_t)0xf40 << (64 - 12));
		return;
	}

	assert(n == 1);

	dst->lo = src->slots[0];
	dst->hi = 1;
}

static force_inline unsigned int peek(const passwdqc_filter_packed_t *src)
{
	uint64_t hi = src->hi;

	if (hi <= 1)
		return (unsigned int)hi; /* 0 or 1 */

	unsigned int ssi = hi >> (64 - 12); /* semi-sort index */

	if (ssi <= 3876)
		return 4;

	return (ssi >> 7) & 3; /* 2 or 3 */
}

static force_inline int kick(passwdqc_filter_unpacked_t *u, passwdqc_filter_i_t i, passwdqc_filter_f_t f, unsigned int size)
{
	uint32_t rnd = i;

	do {
/*
 * Peek at alternate buckets for each of the fingerprints stored in the bucket
 * that we have to kick an entry from.  If one of those buckets isn't full,
 * plan to kick that fingerprint.  Moreover, if a bucket has 2 or more empty
 * slots, don't look further and kick that fingerprint right away.  There are
 * two aspects here: (1) never missing a non-full bucket that is just one step
 * away greatly reduces the number of kicks needed to reach high load factors
 * (approximately from 16x to 6x of capacity for 98% as compared to pure random
 * walk, and twice quicker in terms of real time on a certain machine), and (2)
 * favoring buckets with 2+ empty slots tends to slightly lower the FP rate.
 */
		passwdqc_filter_i_t ia;
		passwdqc_filter_f_t fkick, fdiff = 0;
		unsigned int n, j = size - 1, bestj = 0;
		do {
			fkick = u->slots[j];
			ia = passwdqc_filter_alti(i, fkick, nbuckets);
			if ((n = peek(&packed[ia])) < size) {
				bestj = j;
				if (!j || n < size - 1)
					goto kick;
			}
			fdiff |= f ^ fkick;
		} while (j--);

/* If there are no non-full buckets one step away, resort to random walk */
		if (!bestj) {
/*
 * If our fingerprint to be (re-)inserted is the same as all of those we could
 * have kicked, then we're at or close to the maximum number of duplicates for
 * this fingerprint that we can hold.  Don't (re-)insert this duplicate so that
 * we don't waste many further kicks on a likely failure.  Note that this isn't
 * necessarily the fingerprint insert() was called on now.  We might have
 * already inserted the new fingerprint and if so are now deleting an excessive
 * duplicate of something previously inserted.
 */
			if (unlikely(!fdiff)) {
				header.dupes++;
				return 1;
			}
/*
 * Good randomness is crucial for the random walk.  This simple formula works
 * surprisingly well by mostly reusing variables that we maintain anyway.
 */
			rnd = (rnd + (uint32_t)fdiff) * (uint32_t)header.kicks;
			if (likely(size != 2)) { /* hopefully, compile-time */
				bestj = rnd >> 30;
				while (bestj >= size) /* only if size == 3 */
					bestj = (rnd <<= 2) >> 30;
			} else {
				bestj = rnd >> 31;
			}
		}

		if (likely(bestj)) { /* recompute unless still have */
			fkick = u->slots[bestj];
			ia = passwdqc_filter_alti(i, fkick, nbuckets);
		}

kick:
		u->slots[bestj] = f;
		pack(&packed[i], u, size, 0);

		n = unpack(u, &packed[ia]);
		if (unlikely(n > size))
			return -1;

		if (n < size) {
			u->slots[n++] = fkick;
			pack(&packed[ia], u, n, PACK_MASK_OLD);
			header.inserts++;
			header.kicks++;
			return 0;
		}

		f = fkick;
		i = ia;
	} while (likely(++header.kicks < maxkicks));

	return -2;
}

static inline int insert(passwdqc_filter_hash_t *h)
{
	passwdqc_filter_i_t i = passwdqc_filter_h2i(h, nbuckets);
	passwdqc_filter_f_t f = passwdqc_filter_h2f(h);

/*
 * Plan to put this entry into the primary bucket if it's below the threshold.
 * Otherwise see if the secondary bucket is less full and use it if so.  This
 * logic balances between two conflicting goals: letting us skip the secondary
 * bucket on lookup when primary isn't full (or is below threshold), and
 * filling different buckets across the entire table evenly.  Each of these
 * goals has two (luckily non-conflicting) sub-goals.  The former reduces FP
 * rate through comparing against fewer stored fingerprints, and speeds up
 * lookups.  The latter helps reach high load factors in fewer kicks and
 * preserves more of the larger fingerprints by not putting unnecessarily many
 * entries in one bucket while we could still avoid that, which also reduces
 * FP rate.  In terms of FP rate, different thresholds turn out to be optimal
 * depending on target load factor: a threshold of 4 is more optimal for the
 * highest load factors (near the maximum of 98%), lower thresholds like 2 are
 * more optimal at lower load factors.  Our gradual increase of effective
 * bucket size plays a further role (even more important at low load factors).
 */
	unsigned int n = peek(&packed[i]);
	if (n >= GET_THRESHOLD) {
		passwdqc_filter_i_t ia = passwdqc_filter_alti(i, f, nbuckets);
		if (peek(&packed[ia]) < n)
			i = ia;
	}

	passwdqc_filter_unpacked_t u;
	n = unpack(&u, &packed[i]);
	if (unlikely(n > GET_BUCKET_SIZE))
		return -1;

	if (n < GET_BUCKET_SIZE) {
		u.slots[n++] = f;
		pack(&packed[i], &u, n, PACK_MASK_ALL);
		header.inserts++;
		return 0;
	}

/*
 * At this point, we have one unpacked bucket that is at exactly the current
 * bucket size.  We could have chosen either primary or secondary at random,
 * as the classic cuckoo filter insertion algorithm does, but testing shows
 * that this is unnecessary and a fixed implementation-specific choice works
 * just as well.
 */

	if (likely(n == 4)) { /* specialized code as an optimization */
/*
 * We only kick fingerprints from full buckets, which implies that they're
 * already masked to the worst extent possible at the current bucket size.
 * This lets us use optimized non-masking pack() in kick()'s loop, but only as
 * long as we don't need the masking for the new fingerprint as well.  Let's
 * pre-mask it here to make this so.  We already know we'll have to insert it
 * into a full bucket (kicking another fingerprint from it), so we couldn't
 * have preserved those bits anyway.
 */
		f &= ((passwdqc_filter_f_t)1 << 33) - 1;
		return kick(&u, i, f, 4);
	} else if (likely(n == 2)) { /* and no bucket is larger yet */
		f &= ((passwdqc_filter_f_t)1 << 61) - 1;
		return kick(&u, i, f, 2);
	} else { /* n == 3 and no bucket is larger yet */
		f &= ((passwdqc_filter_f_t)1 << 41) - 1;
		return kick(&u, i, f, 3);
	}
}

static const uint8_t fingerprint_sizes_234[] = {61, 41, 33};
static const char * const hash_names[] = {"opaque", "MD4", "NTLM CP1252"};

static void print_status(void)
{
	printf("Capacity %llu, usage %llu (inserts %llu, deletes %llu), load %.2f%%\n"
	    "Hash type %s, buckets of %u at least %u-bit fingerprints, threshold %u\n"
	    "Effective duplicates omitted %llu, kicks %llu (%.2f of capacity)\n",
	    (unsigned long long)header.capacity, (unsigned long long)(header.inserts - header.deletes),
	    (unsigned long long)header.inserts, (unsigned long long)header.deletes,
	    100. * (header.inserts - header.deletes) / header.capacity,
	    header.hash_id < sizeof(hash_names) / sizeof(hash_names[0]) ? hash_names[header.hash_id] : "unsupported",
	    (unsigned int)header.bucket_size, (unsigned int)fingerprint_sizes_234[header.bucket_size - 2],
	    (unsigned int)header.threshold,
	    (unsigned long long)header.dupes, (unsigned long long)header.kicks,
	    1. * header.kicks / header.capacity);
}

static int new_filter(void)
{
	header.capacity = (header.capacity + 3) & ~3ULL;
	nbuckets = (uint32_t)(header.capacity >> 2);
	packed = calloc(nbuckets, sizeof(*packed));
	if (!packed) {
		perror("pwqfilter: calloc");
		return -1;
	}

	memcpy(header.version, PASSWDQC_FILTER_VERSION, sizeof(header.version));
	if (options & OPT_FP_RATE_AT_HIGH_LOAD)
		SET_THRESHOLD(header.threshold = 4)
	else
		SET_THRESHOLD(header.threshold = 2)
	SET_BUCKET_SIZE(header.bucket_size = header.threshold)
	header.hash_id = (options & OPT_HASH_ID_MASK) >> OPT_HASH_ID_SHIFT;
	header.endianness = PASSWDQC_FILTER_ENDIANNESS;

	return 0;
}

static int read_filter(const char *filename, int print_status_only)
{
	FILE *f = fopen(filename, "rb");
	if (!f) {
		perror("pwqfilter: fopen");
		return -1;
	}

	int retval = 0;
	if (fread(&header, sizeof(header), 1, f) != 1)
		goto fail_fread;

	if (passwdqc_filter_verify_header(&header)) {
		fprintf(stderr, "pwqfilter: Invalid or unsupported input filter.\n");
		goto fail;
	}

	if ((options & OPT_VERBOSE) || print_status_only) {
		print_status();
		if (print_status_only)
			goto out;
	}

	SET_THRESHOLD(header.threshold)
	SET_BUCKET_SIZE(header.bucket_size)

	if ((options & OPT_FP_RATE_AT_HIGH_LOAD) && header.threshold < 4)
		fprintf(stderr, "pwqfilter: Warning: --optimize-fp-rate-at-high-load is too late for this filter.\n");

	nbuckets = (uint32_t)(header.capacity >> 2);
	if (SIZE_MAX <= 0xffffffffU && nbuckets > SIZE_MAX / sizeof(*packed)) {
		fprintf(stderr, "pwqfilter: Input filter claims to be too large for this system.\n");
		goto fail;
	}

	packed = malloc(nbuckets * sizeof(*packed));
	if (!packed) {
		perror("pwqfilter: malloc");
		goto fail;
	}

	if (fread(packed, sizeof(*packed), nbuckets, f) != nbuckets) {
fail_fread:
		if (ferror(f))
			perror("pwqfilter: fread");
		else
			fprintf(stderr, "pwqfilter: fread: Unexpected EOF\n");
fail:
		retval = -1;
	}

out:
	fclose(f);

	return retval;
}

static int write_filter(const char *filename)
{
	FILE *f = fopen(filename, "wb");
	if (!f) {
		perror("pwqfilter: fopen");
		return -1;
	}

	int retval = 0;
	if (fwrite(&header, sizeof(header), 1, f) != 1 ||
	    fwrite(packed, sizeof(*packed), nbuckets, f) != nbuckets) {
		perror("pwqfilter: fwrite");
		retval = -1;
	}

	if (fclose(f) || retval) {
		if (!retval)
			perror("pwqfilter: fclose");
		retval = -1;
		if (unlink(filename))
			perror("pwqfilter: unlink");
	}

	return retval;
}

#define READ_LINE_MAX 8192

static inline char *read_line(void)
{
#ifdef __GNUC__
__attribute__ ((aligned (128)))
#endif
	static char buf[READ_LINE_MAX + 2];

	buf[READ_LINE_MAX] = '\n';

	if (unlikely(!fgets(buf, sizeof(buf), stdin))) {
		if (ferror(stdin))
			perror("pwqfilter: fgets");
		return NULL;
	}

	if (unlikely(buf[READ_LINE_MAX] != '\n')) {
		int c;
		do {
			c = getc(stdin);
		} while (c != EOF && c != '\n');
		if (ferror(stdin)) {
			perror("pwqfilter: getc");
			return NULL;
		}
	}

	return buf;
}

static inline int unhex(passwdqc_filter_hash_t *dst, const char *src)
{
#ifdef __GNUC__
__attribute__ ((aligned (64)))
#endif
	static const uint8_t a2i[] = {
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
		16, 16, 16, 16, 16, 16, 16,
		10, 11, 12, 13, 14, 15,
		16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
		16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
		10, 11, 12, 13, 14, 15
	};
	unsigned char *dp = dst->uc;
	const unsigned char *dend = dst->uc + sizeof(dst->uc);
	const unsigned char *sp = (const unsigned char *)src;

	do {
		unsigned int c, hi, lo;
		c = *sp++ - '0';
		if (c >= sizeof(a2i) || (hi = a2i[c]) > 15)
			break;
		c = *sp++ - '0';
		if (c >= sizeof(a2i) || (lo = a2i[c]) > 15)
			break;
		*dp++ = (hi << 4) | lo;
	} while (likely(dp < dend));

	return likely(dp == dend) ? 0 : -1;
}

static inline int line_to_hash(passwdqc_filter_hash_t *dst, const char *line, unsigned long long lineno)
{
	if (options & OPT_HASH_ALL) {
		if (unlikely(line[READ_LINE_MAX] != '\n')) {
			fprintf(stderr, "\rpwqfilter: Line %llu too long.\n", lineno);
			return -1;
		}
		if (options & OPT_HASH_MD4)
			passwdqc_filter_md4(dst, line);
		else
			passwdqc_filter_ntlm_cp1252(dst, line);
	} else if (unlikely(unhex(dst, line))) {
		fprintf(stderr, "\rpwqfilter: Not a supported hex-encoded hash on standard input line %llu.\n", lineno);
		return -1;
	}

	return 0;
}

static int lookup_loop(void)
{
	char *line;
	unsigned long long lineno = 0, lookups = 0, positive = 0, negative = 0, errors = 0;

	while ((line = read_line())) {
		lineno++;

		passwdqc_filter_hash_t h;
		if (unlikely(line_to_hash(&h, line, lineno))) {
			errors++;
			continue;
		}

		lookups++;
		int status = lookup(&h, ~(passwdqc_filter_f_t)0);
		if (unlikely(status < 0))
			break;
		if (status) {
			positive++;
			if (!(options & (OPT_COUNT | OPT_INVERT_MATCH))) {
print:
				if (options & OPT_LINE_NUMBER)
					printf("%llu:", lineno);
				fputs(line, stdout);
			}
		} else {
			negative++;
			if ((options & (OPT_COUNT | OPT_INVERT_MATCH)) == OPT_INVERT_MATCH)
				goto print;
		}
	}

	if (line)
		fprintf(stderr, "Data corruption detected, abandoning further search\n");
	else if (options & OPT_COUNT)
		printf("%llu\n", (options & OPT_INVERT_MATCH) ? negative : positive);
	if (options & OPT_VERBOSE)
		fprintf(stderr, "Lines %llu, lookups %llu, positive %llu, negative %llu, errors %llu\n",
		    lineno, lookups, positive, negative, errors);

	if (line || ferror(stdin))
		return -1;

	return !!((options & OPT_INVERT_MATCH) ? negative : positive);
}

static void set_bucket_size(void)
{
	uint64_t usage = header.inserts - header.deletes;
	uint64_t max_kicks_until_size_3 = (header.capacity >> ((options & OPT_FP_RATE) ? 2 : 5)) * 3;
	unsigned int size = 4;
	if (usage < header.capacity * 44 / 100 && header.kicks <= max_kicks_until_size_3)
		size = 2;
	else if (usage < header.capacity * 71 / 100 && header.kicks <= (max_kicks_until_size_3 << 1))
		size = 3;

	if (size < GET_THRESHOLD)
		size = GET_THRESHOLD;

	if (size > GET_BUCKET_SIZE || !header.inserts) {
		if (size > GET_BUCKET_SIZE)
			SET_BUCKET_SIZE((header.bucket_size = size))
		if (options & OPT_VERBOSE) {
			putc('\r', stderr);
			printf("Storing at least %u-bit fingerprints since load %.2f%%, kicks %.2f of capacity\n",
			    (unsigned int)fingerprint_sizes_234[GET_BUCKET_SIZE - 2],
			    100. * (header.inserts - header.deletes) / header.capacity,
			    1. * header.kicks / header.capacity);
		}
	}
}

static void print_progress(unsigned long long lineno)
{
	fprintf(stderr, "\rLines %.*f%s, load %.2f%%, kicks %.2f of capacity",
	    lineno < 1000000 ? 0 : 3,
	    lineno < 1000000 ? (double)lineno : 1e-6 * lineno,
	    lineno < 1000000 ? "" : "M",
	    100. * (header.inserts - header.deletes) / header.capacity,
	    1. * header.kicks / header.capacity);
}

static int insert_loop(void)
{
	uint64_t inserts_start = header.inserts;
	uint64_t dupes_start = header.dupes;

	uint64_t checkpoint = 0, previous = 0;
	uint64_t effort_step = (header.capacity + 199) / 200;
	uint64_t inserts_step = effort_step;
	uint64_t inserts_goal = header.capacity / 10;
	if (inserts_goal < header.inserts)
		inserts_goal = header.inserts;
	maxkicks = header.capacity;

	int status = 0;
	char *line;
	unsigned long long lineno = 0, errors = 0;

/*
 * A threshold of 0 is different for lookup, but we can optimize its handling
 * for insert.
 */
	if (GET_THRESHOLD == 0)
		SET_THRESHOLD(1)

	while ((line = read_line())) {
		uint64_t effort = header.inserts + header.kicks;
		if (unlikely(effort >= checkpoint)) {
			set_bucket_size();
			if (!checkpoint || effort - previous >= 1000000) {
				previous = effort;
				print_progress(lineno);
			}
			checkpoint = effort + effort_step;
			if (header.inserts >= inserts_goal) {
				uint64_t usage = header.inserts - header.deletes;
				if (usage > header.capacity)
					break;
				if (usage >= header.capacity * 97 / 100)
					inserts_step = (header.capacity + 999 - usage) / 1000;
				else
					inserts_step = (header.capacity + 199 - usage) / 200;
				inserts_goal = header.inserts + inserts_step;
				maxkicks = header.kicks + header.capacity;
			}
		}

		lineno++;

		passwdqc_filter_hash_t h;
		if (unlikely(line_to_hash(&h, line, lineno))) {
			errors++;
			continue;
		}

		if (unlikely((status = insert(&h)) < 0))
			break;
	}

	SET_THRESHOLD(header.threshold)

	if (line) {
		print_progress(lineno);
		if (status == -2) {
/*
 * We have to abandon the filter here because when we bump into maxkicks we've
 * kicked out and not re-inserted an entry likely other than the one we were
 * trying to insert originally.  To avoid this, we'd need a separate soft limit
 * that we'd most likely bump into between insert() calls (not inside a call).
 */
			fprintf(stderr, "\nProgress almost stalled, abandoning incomplete filter\n");
/*
 * For filters of medium size (some million entries), we expect to be able to
 * achieve a little over 98% (e.g., 98.03%) with unbiased non-repeating inputs.
 * For small filters, there's significant variability of maximum achievable
 * load (e.g., 97.7% to 98.3%).  For filters approaching the maximum supported
 * capacity of almost 2^34, the biases caused by our use of only 32 bits in
 * h2i() become significant and in simulation limit the achievable load e.g. to
 * 97% for a capacity of a little over half the maximum.  To be on the safe
 * side, we only print a likely explanation for below 97% and only for filters
 * that are way below the maximum capacity.
 */
			if (header.capacity <= (1ULL << 32) &&
			    header.inserts - header.deletes < header.capacity * 97 / 100)
				fprintf(stderr, "Likely too many repeating%s inputs%s\n",
				    (options & OPT_HASH_ALL) ? "" : " or biased",
				    header.capacity < 1000000 ? " or filter is too small" : "");
		} else { /* -1 return from insert() or usage > capacity */
			fprintf(stderr, "\nData corruption detected, abandoning incomplete filter\n");
		}
	}
	fprintf(stderr, "\rLines %llu, inserts %llu, excessive effective duplicates %llu, errors %llu\n",
	    lineno, (unsigned long long)(header.inserts - inserts_start), (unsigned long long)(header.dupes - dupes_start), errors);

	return (line || ferror(stdin)) ? -1 : 0;
}

static int test_fp_rate(void)
{
	unsigned int fps = 0, tests = 0, errors = 0;

	if (header.inserts != header.deletes)
	do {
		int i, n = tests + (1 << 22); /* signed int for OpenMP 2.5 */
#ifdef _OPENMP
#pragma omp parallel for default(none) private(i) shared(n, fps, tests, errors)
#endif
		for (i = tests; i < n; i++) {
			passwdqc_filter_hash_t h;
			MD4_CTX ctx;
			MD4_Init(&ctx);
			ctx.a += i;
			MD4_Update(&ctx, "notNTLM", 8);
			MD4_Final(h.uc, &ctx);

/*
 * Process the hash table semi-sequentially for some speedup.  As long as we
 * ensure we test all possible values of the first 3 bytes, this does not bias
 * the final estimate, but the verbose output shown during testing might show
 * biased numbers until eventually converging to the global average.  See also
 * the comment in passwdqc_filter_h2i().
 */
			h.uc[0] = i >> 22;
			h.uc[1] = i >> 14;
			h.uc[2] = i >> 6;
			h.u32[0] = ((h.u32[0] & 0x0f0f0f0f) << 4) | ((h.u32[0] >> 4) & 0x0f0f0f0f);

			switch (lookup(&h, ~(passwdqc_filter_f_t)0xfffff)) {
			case 0:
				break;
			case 1:
#ifdef _OPENMP
#pragma omp atomic
#endif
				fps++;
				break;
			default: /* -1 */
#ifdef _OPENMP
#pragma omp atomic
#endif
				errors++;
			}
#ifndef _OPENMP
			if (unlikely(errors))
				break;
#endif
		}
		tests = n;

		double progress = 100. * tests / (1 << 30);
		if (options & OPT_VERBOSE)
			fprintf(stderr, "\rTests %u (%.2f%%), FPs %u (rate %.3e) for fingerprints cut by 20 bits",
			    tests, progress, fps, (double)fps / tests);
		else
			fprintf(stderr, "\rTests %u (%.2f%%)", tests, progress);
	} while (tests < (1 << 30) && !errors);

	if (tests)
		putc('\n', stderr);
	if (errors) {
		fprintf(stderr, "Data corruption detected, abandoning further testing\n");
		return -1;
	}
	if (fps) {
		double bperfp = 1e-9 * ((unsigned long long)tests << 20) / fps;
		printf("Estimated FP rate 1 in %.*f billion\n", (bperfp < 10) + (bperfp < 100) + (bperfp < 1000), bperfp);
	} else {
		printf("Estimated FP rate 0 (%s)\n", tests ? "no FPs seen in testing" : "empty filter");
	}

	return 0;
}

static int opt_eq(const char *ref, const char *opt, const char **arg)
{
	size_t n = strlen(ref);
	int retval = !strncmp(ref, opt, n) && (!opt[n] || opt[n] == '=');
	if (retval && opt[n] && opt[n + 1])
		*arg = &opt[n + 1];
	return retval;
}

static void print_help(void)
{
	puts("Manage binary passphrase filter files.\n"
	    "\nUsage: pwqfilter [options]\n"
	    "\nValid options are:\n"
	    "Modes\n"
	    "  --lookup (default)\n"
	    "       lookup plaintexts or hashes against an existing filter;\n"
	    "  --status\n"
	    "       print usage statistics for an existing filter;\n"
	    "  --create=CAPACITY\n"
	    "       create a new filter for up to ~98% of CAPACITY entries;\n"
	    "  --insert\n"
	    "       insert entries into an existing filter;\n"
	    "  --test-fp-rate (can be used on its own or along with another mode)\n"
	    "       estimate the false positive rate (FP rate) of a filter;\n"
	    "Optimizations (with --create or --insert)\n"
	    "  --optimize-fp-rate\n"
	    "       better than default FP rate, briefly slower inserts after ~30% and ~60%;\n"
	    "  --optimize-fp-rate-at-high-load\n"
	    "       better than default FP rate at load ~95% to 98%, a lot worse below ~90%;\n"
	    "Input and output\n"
	    "  -f FILE or --filter=FILE\n"
	    "       read an existing filter from FILE;\n"
	    "  -o FILE or --output=FILE\n"
	    "       write a new or modified filter to FILE;\n"
	    "  --pre-hashed (default for filters created with this option and no --hash-*)\n"
	    "       lookup or insert by hex-encoded hashes, not plaintexts;\n"
	    "  --hash-md4 (default for new filters)\n"
	    "       hash plaintexts with MD4 prior to lookup or insert;\n"
	    "  --hash-ntlm-cp1252\n"
	    "       hash assumed CP1252 plaintexts with NTLM prior to lookup or insert;\n"
	    "Lookup output modifiers\n"
	    "  -c or --count\n"
	    "       print a count of (non-)matching lines instead of the lines themselves;\n"
	    "  -n or --line-number\n"
	    "       prefix each line with its number in the input stream;\n"
	    "  -v or --invert-match\n"
	    "       print or count non-matching lines;\n"
	    "General\n"
	    "  --verbose\n"
	    "       print additional information;\n"
	    "  --version\n"
	    "       print program version and exit;\n"
	    "  -h or --help\n"
	    "       print this help text and exit.");
}

int main(int argc, char **argv)
{
	enum {MODE_NONE = 0, MODE_LOOKUP = 1, MODE_STATUS = 2, MODE_CREATE = 3, MODE_INSERT} mode = MODE_NONE;
	const char *input = NULL, *output = NULL;

	options = 0;

	if (unlikely(argc <= 1)) {
		fprintf(stderr, "pwqfilter: No action requested, try --help.\n");
		return 2;
	}

	while (argc > 1) {
		const char *opt = argv[1], *arg = NULL;
		if (opt[0] == '-' && opt[1] != '-' && opt[1] && opt[2]) {
			static char optbuf[3] = {'-', 0, 0};
			optbuf[1] = opt[1];
			opt = optbuf;
			memmove(&argv[1][1], &argv[1][2], strlen(&argv[1][1]));
		} else {
			argc--; argv++;
		}

		if (!strcmp("-h", opt) || !strcmp("--help", opt)) {
			print_help();
			return 0;
		}

		if (!strcmp("--version", opt)) {
			printf("pwqfilter version %s\n", PASSWDQC_VERSION);
			return 0;
		}

		if (!strcmp("--lookup", opt)) {
			if (mode || output)
				goto fail_conflict;
			mode = MODE_LOOKUP;
			continue;
		}

		if (!strcmp("--status", opt)) {
			if (mode || (options & (OPT_HASH_ALL | OPT_PRE_HASHED)))
				goto fail_conflict;
			mode = MODE_STATUS;
			continue;
		}

		if (opt_eq("--create", opt, &arg)) {
			if (mode || input || (options & OPT_LOOKUP))
				goto fail_conflict;
			mode = MODE_CREATE;
			if (!arg)
				goto fail_no_arg;
			char *e;
			header.capacity = strtoul(arg, &e, 0);
			if (*e || !header.capacity || header.capacity > ((1ULL << 32) - 1) * 4) {
				fprintf(stderr, "pwqfilter: Requested capacity is invalid or unsupported.\n");
				return 2;
			}
			continue;
		}

		if (!strcmp("--insert", opt)) {
			if (mode || (options & OPT_LOOKUP))
				goto fail_conflict;
			mode = MODE_INSERT;
			continue;
		}

		if (!strcmp("--test-fp-rate", opt)) {
			options |= OPT_TEST_FP_RATE;
			continue;
		}

		if (!strcmp("--optimize-fp-rate", opt)) {
			if (options & OPT_FP_RATE_AT_HIGH_LOAD)
				goto fail_conflict;
			options |= OPT_FP_RATE;
			continue;
		}

		if (!strcmp("--optimize-fp-rate-at-high-load", opt)) {
			if (options & OPT_FP_RATE)
				goto fail_conflict;
			options |= OPT_FP_RATE_AT_HIGH_LOAD;
			continue;
		}

		if (!strcmp("-f", opt) || opt_eq("--filter", opt, &arg)) {
			if (mode == MODE_CREATE || input)
				goto fail_conflict;
			if (!opt[2]) {
				argc--;
				arg = *++argv;
			}
			if (!arg)
				goto fail_no_arg;
			input = arg;
			continue;
		}

		if (!strcmp("-o", opt) || opt_eq("--output", opt, &arg)) {
			if (mode == MODE_LOOKUP || mode == MODE_STATUS || output)
				goto fail_conflict;
			if (!opt[2]) {
				argc--;
				arg = *++argv;
			}
			if (!arg)
				goto fail_no_arg;
			output = arg;
			continue;
		}

		if (!strcmp("--pre-hashed", opt)) {
			if (mode == MODE_STATUS)
				goto fail_conflict;
			options |= OPT_PRE_HASHED;
			continue;
		}

		if (!strcmp("--hash-md4", opt)) {
			if ((options & OPT_HASH_ALL) || mode == MODE_STATUS)
				goto fail_conflict;
			options |= OPT_HASH_MD4;
			continue;
		}

		if (!strcmp("--hash-ntlm-cp1252", opt)) {
			if ((options & OPT_HASH_ALL) || mode == MODE_STATUS)
				goto fail_conflict;
			options |= OPT_HASH_NTLM_CP1252;
			continue;
		}

		if (!strcmp("-c", opt) || !strcmp("--count", opt)) {
			if (mode > MODE_LOOKUP || (options & OPT_LINE_NUMBER))
				goto fail_conflict;
			options |= OPT_COUNT;
			continue;
		}

		if (!strcmp("-n", opt) || !strcmp("--line-number", opt)) {
			if (mode > MODE_LOOKUP || (options & OPT_COUNT))
				goto fail_conflict;
			options |= OPT_LINE_NUMBER;
			continue;
		}

		if (!strcmp("-v", opt) || !strcmp("--invert-match", opt)) {
			if (mode > MODE_LOOKUP)
				goto fail_conflict;
			options |= OPT_INVERT_MATCH;
			continue;
		}

		if (!strcmp("--verbose", opt)) {
			options |= OPT_VERBOSE;
			continue;
		}

		fprintf(stderr, "pwqfilter: Option %s unrecognized.\n", opt);
		return 2;

fail_no_arg:
		fprintf(stderr, "pwqfilter: Option %s requires an argument.\n", opt);
		return 2;

fail_conflict:
		fprintf(stderr, "pwqfilter: Option %s conflicts with previously specified options.\n", opt);
		return 2;
	}

	if (!mode) {
		if (options & OPT_TEST_FP_RATE) {
			if (options & (OPT_HASH_ALL | OPT_PRE_HASHED))
				goto fail_unused;
		} else {
			mode = MODE_LOOKUP; /* default mode */
		}
	}

	if (!input && !(options & (OPT_HASH_ALL | OPT_PRE_HASHED)))
		options |= OPT_HASH_MD4; /* default hash type */

	if (mode <= MODE_STATUS && output) {
		fprintf(stderr, "pwqfilter: No filter modifications requested yet an output filename specified.\n");
		return 2;
	}

	if ((mode != MODE_LOOKUP && (options & OPT_LOOKUP)) ||
	    (mode < MODE_CREATE && (options & OPT_INSERT)) ||
	    (mode != MODE_CREATE && (options & OPT_HASH_ALL) && (options & OPT_PRE_HASHED))) {
fail_unused:
		fprintf(stderr, "pwqfilter: The requested mode doesn't use other specified options.\n");
		return 2;
	}

	if (mode != MODE_CREATE && !input) {
		fprintf(stderr, "pwqfilter: Neither requested to create a new filter nor to use an existing one.\n");
		return 2;
	}

	if (mode > MODE_STATUS && !output)
		fprintf(stderr, "pwqfilter: No output filename specified - doing a dry run.\n");

	if ((input && read_filter(input, mode == MODE_STATUS)) || (!input && new_filter()))
		return 2;

/*
 * The uses of (un)likely() here optimize for --create --pre-hashed.  Somehow
 * omitting them results in very different code (smaller and slower) in inner
 * loops at least on a certain RHEL7'ish test system.
 */
	if (unlikely(mode == MODE_STATUS)) {
		if ((options & OPT_TEST_FP_RATE) && test_fp_rate())
			return 2;
		return 0;
	}

	if (!likely(options & OPT_PRE_HASHED)) {
		if (header.hash_id > PASSWDQC_FILTER_HASH_MAX) {
			fprintf(stderr, "pwqfilter: Input filter claims unsupported hash type.\n");
			return 2;
		}

		if (header.hash_id != PASSWDQC_FILTER_HASH_OPAQUE) {
			uint32_t new_options = (options & ~OPT_HASH_ID_MASK) | ((uint32_t)header.hash_id << OPT_HASH_ID_SHIFT);
			if ((options & OPT_HASH_ALL) && new_options != options) {
				fprintf(stderr, "pwqfilter: Input filter's hash type is different than requested.\n");
				return 2;
			}
			options = new_options;
		}
	}

	ssinit();

	if (mode == MODE_LOOKUP) {
		int status = 1 - lookup_loop();
		if ((options & OPT_TEST_FP_RATE) && test_fp_rate())
			return 2;
		return status;
	}

	if (likely(mode >= MODE_CREATE)) {
/*
 * The weird combination of --pre-hashed and --hash* is allowed with --create
 * for writing the claimed hash type into the filter, but shouldn't result in
 * us hashing the hashes.
 */
		if (options & OPT_PRE_HASHED)
			options &= ~OPT_HASH_ALL;

		if (insert_loop())
			return 2;

		if (options & OPT_VERBOSE)
			print_status();

		if (output && write_filter(output))
			return 2;
	}

	if ((options & OPT_TEST_FP_RATE) && test_fp_rate())
		return 2;

	return 0;
}
