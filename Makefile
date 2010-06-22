#
# Copyright (c) 2000-2003,2005,2009,2010 by Solar Designer
# Copyright (c) 2008,2009 by Dmitry V. Levin
# See LICENSE
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
MAN1 = pwqgen.1 pwqcheck.1
MAN5 = passwdqc.conf.5
MAN8 = $(TITLE).8
MANMODE = 644
BINDIR = /usr/bin
BINMODE = 755
CONFDIR = /etc
CONFMODE = 644
SHARED_LIBDIR = /lib
SHARED_LIBDIR_SUN = /usr/lib
SHARED_LIBDIR_REL = ../..$(SHARED_LIBDIR)
DEVEL_LIBDIR = /usr/lib
SECUREDIR = /lib/security
SECUREDIR_SUN = /usr/lib/security
INCLUDEDIR = /usr/include
MANDIR = /usr/share/man
DESTDIR =

CC = gcc
LD = $(CC)
LD_lib = $(LD)
RM = rm -f
LN_s = ln -s -f
MKDIR = umask 022 && mkdir -p
INSTALL = install -c
# We support Sun's older /usr/ucb/install, but not the newer /usr/sbin/install.
INSTALL_SUN = /usr/ucb/install -c
CFLAGS = -Wall -W -O2
CFLAGS_lib = $(CFLAGS) -fPIC
CFLAGS_bin = $(CFLAGS) -fomit-frame-pointer

LDFLAGS =
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
LDFLAGS_pam_SUN = $(LDFLAGS_shared_SUN)
LDFLAGS_pam_HP = $(LDFLAGS_shared_HP)

LDLIBS_lib =
LDLIBS_pam = -lpam -lcrypt
LDLIBS_pam_LINUX = -lpam -lcrypt
LDLIBS_pam_SUN = -lpam -lcrypt
LDLIBS_pam_HP = -lpam -lsec

# Uncomment this to use cc instead of gcc
#CC = cc
# Uncomment this to use Sun's C compiler flags
#CFLAGS = -xO2
#CFLAGS_lib = $(CFLAGS) -KPIC
#CFLAGS_bin = $(CFLAGS)
# Uncomment this to use HP's ANSI C compiler flags
#CFLAGS = -Ae +w1 +W 474,486,542 +O2
#CFLAGS_lib = $(CFLAGS) +z
#CFLAGS_bin = $(CFLAGS)

CONFIGS = passwdqc.conf
BINS = pwqgen pwqcheck
PROJ = $(SHARED_LIB) $(DEVEL_LIB) $(SHARED_PAM) $(BINS)
OBJS_LIB = concat.o passwdqc_check.o passwdqc_load.o passwdqc_parse.o passwdqc_random.o wordset_4k.o
OBJS_PAM = pam_passwdqc.o
OBJS_GEN = pwqgen.o
OBJS_CHECK = pwqcheck.o

default: all

all pam utils install install_lib install_pam install_utils uninstall remove remove_lib remove_pam remove_utils:
	case "`uname -s`" in \
	Linux)	$(MAKE) CFLAGS_lib="$(CFLAGS_lib) -DHAVE_SHADOW" \
			LDFLAGS_lib="$(LDFLAGS_lib_LINUX)" \
			LDFLAGS_pam="$(LDFLAGS_pam_LINUX)" \
			LDLIBS_pam="$(LDLIBS_pam_LINUX)" \
			$@_wrapped;; \
	SunOS)	$(MAKE) -e CFLAGS_lib="$(CFLAGS_lib) -DHAVE_SHADOW" \
			LD_lib=ld \
			LDFLAGS_lib="$(LDFLAGS_lib_SUN)" \
			LDFLAGS_pam="$(LDFLAGS_pam_SUN)" \
			LDLIBS_pam="$(LDLIBS_pam_SUN)" \
			INSTALL="$(INSTALL_SUN)" \
			SHARED_LIBDIR="$(SHARED_LIBDIR_SUN)" \
			SECUREDIR="$(SECUREDIR_SUN)" \
			$@_wrapped;; \
	HP-UX)	$(MAKE) CFLAGS_lib="$(CFLAGS_lib) -DHAVE_SHADOW" \
			LD_lib=ld \
			LDFLAGS_lib="$(LDFLAGS_lib_HP)" \
			LDFLAGS_pam="$(LDFLAGS_pam_HP)" \
			LDLIBS_pam="$(LDLIBS_pam_HP)" \
			$@_wrapped;; \
	*)	$(MAKE) $@_wrapped;; \
	esac

all_wrapped: pam_wrapped utils_wrapped

pam_wrapped: $(SHARED_PAM)

utils_wrapped: $(BINS)

$(SHARED_LIB): $(OBJS_LIB) $(MAP_LIB)
	$(LD_lib) $(LDFLAGS_lib) $(OBJS_LIB) $(LDLIBS_lib) -o $(SHARED_LIB)

$(DEVEL_LIB): $(SHARED_LIB)
	$(LN_s) $(SHARED_LIB) $(DEVEL_LIB)

$(SHARED_PAM): $(OBJS_PAM) $(MAP_PAM) $(DEVEL_LIB)
	$(LD_lib) $(LDFLAGS_pam) $(OBJS_PAM) $(LDLIBS_pam) -L. -lpasswdqc -o $(SHARED_PAM)

pwqgen: $(OBJS_GEN) $(DEVEL_LIB)
	$(LD) $(LDFLAGS) $(OBJS_GEN) -L. -lpasswdqc -o $@

pwqcheck: $(OBJS_CHECK) $(DEVEL_LIB)
	$(LD) $(LDFLAGS) $(OBJS_CHECK) -L. -lpasswdqc -o $@

pwqgen.o: pwqgen.c passwdqc.h
	$(CC) $(CFLAGS_bin) -c $*.c

pwqcheck.o: pwqcheck.c passwdqc.h
	$(CC) $(CFLAGS_bin) -c $*.c

.c.o:
	$(CC) $(CFLAGS_lib) -c $*.c

concat.o: concat.h
pam_passwdqc.o: passwdqc.h pam_macros.h
passwdqc_check.o: passwdqc.h wordset_4k.h
passwdqc_load.o: passwdqc.h concat.h
passwdqc_parse.o: passwdqc.h concat.h
passwdqc_random.o: passwdqc.h wordset_4k.h
wordset_4k.o: wordset_4k.h

install_wrapped: install_lib_wrapped install_utils_wrapped install_pam_wrapped
	@echo 'Consider running ldconfig(8) to update the dynamic linker cache.'

install_lib_wrapped:
	$(MKDIR) $(DESTDIR)$(CONFDIR)
	$(INSTALL) -m $(CONFMODE) $(CONFIGS) $(DESTDIR)$(CONFDIR)/

	$(MKDIR) $(DESTDIR)$(SHARED_LIBDIR)
	$(INSTALL) -m $(SHLIBMODE) $(SHARED_LIB) $(DESTDIR)$(SHARED_LIBDIR)/

	$(MKDIR) $(DESTDIR)$(DEVEL_LIBDIR)
	$(LN_s) $(SHARED_LIBDIR_REL)/$(SHARED_LIB) \
		$(DESTDIR)$(DEVEL_LIBDIR)/$(DEVEL_LIB)

	$(MKDIR) $(DESTDIR)$(INCLUDEDIR)
	$(INSTALL) -m $(INCMODE) $(HEADER) $(DESTDIR)$(INCLUDEDIR)/

	$(MKDIR) $(DESTDIR)$(MANDIR)/man5
	$(INSTALL) -m $(MANMODE) $(MAN5) $(DESTDIR)$(MANDIR)/man5/

install_utils_wrapped:
	$(MKDIR) $(DESTDIR)$(BINDIR)
	$(INSTALL) -m $(BINMODE) $(BINS) $(DESTDIR)$(BINDIR)/

	$(MKDIR) $(DESTDIR)$(MANDIR)/man1
	$(INSTALL) -m $(MANMODE) $(MAN1) $(DESTDIR)$(MANDIR)/man1/

install_pam_wrapped:
	$(MKDIR) $(DESTDIR)$(SECUREDIR)
	$(INSTALL) -m $(SHLIBMODE) $(SHARED_PAM) $(DESTDIR)$(SECUREDIR)/

	$(MKDIR) $(DESTDIR)$(MANDIR)/man8
	$(INSTALL) -m $(MANMODE) $(MAN8) $(DESTDIR)$(MANDIR)/man8/

uninstall_wrapped remove_wrapped: remove_pam_wrapped remove_utils_wrapped remove_lib_wrapped

remove_pam_wrapped:
	$(RM) $(DESTDIR)$(MANDIR)/man8/$(MAN8)
	$(RM) $(DESTDIR)$(SECUREDIR)/$(SHARED_PAM)

remove_utils_wrapped:
	for f in $(MAN1); do $(RM) $(DESTDIR)$(MANDIR)/man1/$$f; done
	for f in $(BINS); do $(RM) $(DESTDIR)$(BINDIR)/$$f; done

remove_lib_wrapped:
	for f in $(MAN5); do $(RM) $(DESTDIR)$(MANDIR)/man5/$$f; done
	for f in $(HEADER); do $(RM) $(DESTDIR)$(INCLUDEDIR)/$$f; done
	for f in $(DEVEL_LIB); do $(RM) $(DESTDIR)$(DEVEL_LIBDIR)/$$f; done
	for f in $(SHARED_LIB); do $(RM) $(DESTDIR)$(SHARED_LIBDIR)/$$f; done
	for f in $(CONFIGS); do $(RM) $(DESTDIR)$(CONFDIR)/$$f; done

clean:
	$(RM) $(PROJ) *.o

.PHONY: all all_wrapped clean install install_lib install_pam install_utils \
	pam pam_wrapped uninstall remove remove_lib remove_pam remove_utils \
	utils utils_wrapped \
	install_wrapped install_lib_wrapped install_pam_wrapped \
	install_utils_wrapped \
	remove_wrapped remove_lib_wrapped remove_pam_wrapped \
	remove_utils_wrapped
