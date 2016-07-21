/*
 * Copyright (c) 2016 by Solar Designer.  See LICENSE.
 */

#include <string.h>

static void memzero(void *buf, size_t len)
{
	memset(buf, 0, len);
}

void (*_passwdqc_memzero)(void *, size_t) = memzero;
