/*
 * Copyright (c) 2020,2021 by Solar Designer
 * See LICENSE
 */

#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE 1
#define _LARGEFILE64_SOURCE 1
#define _LARGE_FILES 1

#include <limits.h>
#include <unistd.h>
#include <fcntl.h>

#define PASSWDQC_FILTER_INTERNALS
#include "passwdqc_filter.h"

static ssize_t read_loop(int fd, void *buffer, size_t count)
{
	ssize_t offset, block;

	offset = 0;
	while (count > 0 && count <= SSIZE_MAX) {
		block = read(fd, (char *)buffer + offset, count);

		if (block < 0)
			return block;
		if (!block)
			return offset;

		offset += block;
		count -= block;
	}

	return offset;
}

int passwdqc_filter_open(passwdqc_filter_t *flt, const char *filename)
{
	if ((flt->fd = open(filename, O_RDONLY)) < 0)
		return -1;

	if (read_loop(flt->fd, &flt->header, sizeof(flt->header)) != sizeof(flt->header) ||
	    passwdqc_filter_verify_header(&flt->header) ||
	    flt->header.hash_id < PASSWDQC_FILTER_HASH_MIN || flt->header.hash_id > PASSWDQC_FILTER_HASH_MAX ||
	    (size_t)lseek(flt->fd, 0, SEEK_END) != sizeof(flt->header) + (flt->header.capacity << 2)) {
		passwdqc_filter_close(flt);
		return -1;
	}

	return 0;
}

int passwdqc_filter_close(passwdqc_filter_t *flt)
{
	int retval = close(flt->fd);
	flt->fd = -1;
	return retval;
}

static int check(const passwdqc_filter_t *flt, passwdqc_filter_i_t i, passwdqc_filter_f_t f)
{
	passwdqc_filter_packed_t p;
	if (lseek(flt->fd, sizeof(flt->header) + (off_t)i * sizeof(p), SEEK_SET) < 0 ||
	    read_loop(flt->fd, &p, sizeof(p)) != sizeof(p))
		return -1;

	passwdqc_filter_unpacked_t u;
	unsigned int n = (unsigned int)passwdqc_filter_unpack(&u, &p, NULL);
	if (n > flt->header.bucket_size)
		return -1;

	unsigned int j;
	for (j = 0; j < n; j++)
		if (passwdqc_filter_f_eq(u.slots[j], f, flt->header.bucket_size))
			return 1;

	return (n < flt->header.threshold) ? 0 : 2;
}

int passwdqc_filter_lookup(const passwdqc_filter_t *flt, const char *plaintext)
{
	passwdqc_filter_hash_t h;

	switch (flt->header.hash_id) {
	case PASSWDQC_FILTER_HASH_MD4:
		passwdqc_filter_md4(&h, plaintext);
		break;
	case PASSWDQC_FILTER_HASH_NTLM_CP1252:
		passwdqc_filter_ntlm_cp1252(&h, plaintext);
		break;
	default:
		return -1;
	}

	uint32_t nbuckets = flt->header.capacity >> 2;
	passwdqc_filter_i_t i = passwdqc_filter_h2i(&h, nbuckets);
	passwdqc_filter_f_t f = passwdqc_filter_h2f(&h);

	int retval = check(flt, i, f);
	if (retval <= 1)
		return retval;

	retval = check(flt, passwdqc_filter_alti(i, f, nbuckets), f);
	if (retval <= 1)
		return retval;

	return 0;
}
