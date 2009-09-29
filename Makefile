#
# Copyright (c) 2000-2003,2005 by Solar Designer.  See LICENSE.
#

TITLE = pam_passwdqc
SHARED_LIB = libpasswdqc.so.0
DEVEL_LIB = libpasswdqc.so
MAP_LIB = libpasswdqc.map
PAM_SO_SUFFIX =
SHARED_PAM = $(TITLE).so$(PAM_SO_SUFFIX)
MAP_PAM = pam_passwdqc.map
SHLIBMODE = 755
HEADER = passwdqc.h
INCMODE = 644
MAN5 = passwdqc.conf.5
MAN8 = $(TITLE).8
MANMODE = 644
BINDIR = /usr/bin
BINMODE = 755
CONFDIR = /etc
CONFMODE = 644
SHARED_LIBDIR = /lib
SHARED_LIBDIR_REL = ../..$(SHARED_LIBDIR)
DEVEL_LIBDIR = /usr/lib
SECUREDIR = /lib/security
INCLUDEDIR = /usr/include
MANDIR = /usr/share/man
DESTDIR =

CC = gcc
LD = $(CC)
RM = rm -f
LN_s = ln -s
MKDIR = mkdir -p
INSTALL = install -c
CFLAGS = -Wall -fPIC -O2

LDFLAGS_shared = --shared
LDFLAGS_shared_LINUX = --shared
LDFLAGS_shared_SUN = -G
LDFLAGS_shared_HP = -b
LDFLAGS_lib = $(LDFLAGS_shared)
LDFLAGS_lib_LINUX = $(LDFLAGS_shared_LINUX) \
	-Wl,--soname,$(SHARED_LIB),--version-script,$(MAP_LIB)
LDFLAGS_lib_SUN = $(LDFLAGS_shared_SUN)
LDFLAGS_lib_HP = $(LDFLAGS_shared_HP)
LDFLAGS_pam = $(LDFLAGS_shared)
LDFLAGS_pam_LINUX = $(LDFLAGS_shared_LINUX) \
	-Wl,--version-script,$(MAP_PAM)
LDFLAGS_SUN = $(LDFLAGS_shared_SUN)
LDFLAGS_HP = $(LDFLAGS_shared_HP)

LDLIBS_lib =
LDLIBS_pam = -lpam -lcrypt
LDLIBS_pam_LINUX = -lpam -lcrypt
LDLIBS_pam_SUN = -lpam -lcrypt
LDLIBS_pam_HP = -lpam -lsec

# Uncomment this to use cc instead of gcc
#CC = cc
# Uncomment this to use Sun's C compiler flags
#CFLAGS = -KPIC -xO2
# Uncomment this to use HP's ANSI C compiler flags
#CFLAGS = -Ae +w1 +W 474,486,542 +z +O2

CONFIGS = passwdqc.conf
BINS = pwqgen pwqcheck
PROJ = $(SHARED_LIB) $(DEVEL_LIB) $(SHARED_PAM) $(BINS)
OBJS_LIB = concat.o passwdqc_check.o passwdqc_load.o passwdqc_parse.o passwdqc_random.o wordset_4k.o
OBJS_PAM = pam_passwdqc.o
OBJS_GEN = pwqgen.o
OBJS_CHECK = pwqcheck.o

all:
	case "`uname -s`" in \
	Linux)	$(MAKE) CFLAGS="$(CFLAGS) -DHAVE_SHADOW" \
			LDFLAGS_lib="$(LDFLAGS_lib_LINUX)" \
			LDFLAGS_pam="$(LDFLAGS_pam_LINUX)" \
			LDLIBS_pam="$(LDLIBS_pam_LINUX)" \
			$(PROJ);; \
	SunOS)	$(MAKE) CFLAGS="$(CFLAGS) -DHAVE_SHADOW" \
			LD=ld \
			LDFLAGS_lib="$(LDFLAGS_lib_SUN)" \
			LDFLAGS_pam="$(LDFLAGS_pam_SUN)" \
			LDLIBS_pam="$(LDLIBS_pam_SUN)" \
			$(PROJ);; \
	HP-UX)	$(MAKE) CFLAGS="$(CFLAGS) -DHAVE_SHADOW" \
			LD=ld \
			LDFLAGS_lib="$(LDFLAGS_lib_HP)" \
			LDFLAGS_pam="$(LDFLAGS_pam_HP)" \
			LDLIBS_pam="$(LDLIBS_pam_HP)" \
			$(PROJ);; \
	*)	$(MAKE) $(PROJ);; \
	esac

$(SHARED_LIB): $(OBJS_LIB) $(MAP_LIB)
	$(LD) $(LDFLAGS_lib) $(OBJS_LIB) $(LDLIBS_lib) -o $(SHARED_LIB)

$(DEVEL_LIB): $(SHARED_LIB)
	$(LN_s) $(SHARED_LIB) $(DEVEL_LIB)

$(SHARED_PAM): $(OBJS_PAM) $(MAP_PAM) $(DEVEL_LIB)
	$(LD) $(LDFLAGS_pam) $(OBJS_PAM) $(LDLIBS_pam) -L. -lpasswdqc -o $(SHARED_PAM)

pwqgen: $(OBJS_GEN) $(DEVEL_LIB)
	$(LD) $(OBJS_GEN) -L. -lpasswdqc -o $@

pwqcheck: $(OBJS_CHECK) $(DEVEL_LIB)
	$(LD) $(OBJS_CHECK) -L. -lpasswdqc -o $@

.c.o:
	$(CC) $(CFLAGS) -c $*.c

concat.o: concat.h
pam_passwdqc.o: passwdqc.h pam_macros.h
passwdqc_check.o: passwdqc.h wordset_4k.h
passwdqc_load.o: passwdqc.h concat.h
passwdqc_parse.o: passwdqc.h concat.h
passwdqc_random.o: passwdqc.h wordset_4k.h
pwqgen.o: passwdqc.h
pwqcheck.o: passwdqc.h
wordset_4k.o: wordset_4k.h

install:
	$(MKDIR) $(DESTDIR)$(CONFDIR)
	$(INSTALL) -m $(CONFMODE) $(CONFIGS) $(DESTDIR)$(CONFDIR)/

	$(MKDIR) $(DESTDIR)$(SHARED_LIBDIR)
	$(INSTALL) -m $(SHLIBMODE) $(SHARED_LIB) $(DESTDIR)$(SHARED_LIBDIR)/

	$(MKDIR) $(DESTDIR)$(DEVEL_LIBDIR)
	$(LN_s) $(SHARED_LIBDIR_REL)/$(SHARED_LIB) \
		$(DESTDIR)$(DEVEL_LIBDIR)/$(DEVEL_LIB)

	$(MKDIR) $(DESTDIR)$(BINDIR)
	$(INSTALL) -m $(BINMODE) $(BINS) $(DESTDIR)$(BINDIR)/

	$(MKDIR) $(DESTDIR)$(SECUREDIR)
	$(INSTALL) -m $(SHLIBMODE) $(SHARED_PAM) $(DESTDIR)$(SECUREDIR)/

	$(MKDIR) $(DESTDIR)$(INCLUDEDIR)
	$(INSTALL) -m $(INCMODE) $(HEADER) $(DESTDIR)$(INCLUDEDIR)/

	$(MKDIR) $(DESTDIR)$(MANDIR)/man5
	$(INSTALL) -m $(MANMODE) $(MAN5) $(DESTDIR)$(MANDIR)/man5/

	$(MKDIR) $(DESTDIR)$(MANDIR)/man8
	$(INSTALL) -m $(MANMODE) $(MAN8) $(DESTDIR)$(MANDIR)/man8/

remove:
	$(RM) $(DESTDIR)$(MANDIR)/man8/$(MAN8)
	$(RM) $(DESTDIR)$(INCLUDEDIR)/$(HEADER)
	$(RM) $(DESTDIR)$(SECUREDIR)/$(SHARED_PAM)
	$(RM) $(DESTDIR)$(DEVEL_LIBDIR)/$(DEVEL_LIB)
	$(RM) $(DESTDIR)$(SHARED_LIBDIR)/$(SHARED_LIB)

clean:
	$(RM) $(PROJ) *.o
