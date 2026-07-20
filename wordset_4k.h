/*
 * Written by Solar Designer <solar at openwall.com> and placed in the
 * public domain.
 */

#ifndef WORDSET_4K_H__
#define WORDSET_4K_H__

#define WORDSET_4K_LENGTH_MAX		6

#if defined(__GNUC__) && (__GNUC__ >= 15)
#define _passwdqc_nonstring __attribute__((nonstring))
#else
#define _passwdqc_nonstring
#endif

extern const char _passwdqc_wordset_4k[][WORDSET_4K_LENGTH_MAX] _passwdqc_nonstring;

#endif /* WORDSET_4K_H__ */
