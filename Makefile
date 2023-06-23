#
# Copyright (c) 2000-2003,2005,2009,2010,2020 by Solar Designer
# Copyright (c) 2008,2009,2017 by Dmitry V. Levin
# Copyright (c) 2017 by Oleg Solovyov
# See LICENSE
#

PACKAGE = passwdqc
VERSION = 2.0.3
TITLE = pam_passwdqc
SHARED_LIB = libpasswdqc.so.1
DEVEL_LIB = libpasswdqc.so
SHARED_LIB_DARWIN = libpasswdqc.0.dylib
SHARED_LIB_CYGWIN = cygpasswdqc-0.dll
DEVEL_LIB_DARWIN = libpasswdqc.dylib
DEVEL_LIB_CYGWIN = libpasswdqc.dll.a
MAP_LIB = libpasswdqc.map
PAM_SO_SUFFIX =
SHARED_PAM = $(TITLE).so$(PAM_SO_SUFFIX)
MAP_PAM = pam_passwdqc.map
SHLIBMODE = 755
HEADER = passwdqc.h
INCMODE = 644
PKGCONFIG = passwdqc.pc
PKGCONFMODE = 644
MAN1 = pwqgen.1 pwqcheck.1 pwqfilter.1
MAN3 = libpasswdqc.3 \
       passwdqc_params_reset.3 \
       passwdqc_params_load.3 \
       passwdqc_params_parse.3 \
       passwdqc_params_free.3 \
       passwdqc_check.3 \
       passwdqc_random.3
MAN5 = passwdqc.conf.5
MAN8 = $(TITLE).8
MANMODE = 644
BINDIR = /usr/bin
BINMODE = 755
CONFDIR = /etc
CONFMODE = 644
SHARED_LIBDIR = /lib
SHARED_LIBDIR_CYGWIN = /usr/bin
SHARED_LIBDIR_SUN = /usr/lib
SHARED_LIBDIR_REL = ../..$(SHARED_LIBDIR)
DEVEL_LIBDIR = /usr/lib
SECUREDIR = /lib/security
SECUREDIR_SUN = /usr/lib/security
SECUREDIR_DARWIN = /usr/lib/pam
INCLUDEDIR = /usr/include
PKGCONFIGDIR = $(DEVEL_LIBDIR)/pkgconfig
MANDIR = /usr/share/man
DESTDIR =
LOCALEDIR = /usr/share/locale
LOCALEMODE = 644

LANGUAGES = ru

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
CPPFLAGS =
CPPFLAGS_bin = $(CPPFLAGS)
CPPFLAGS_lib = $(CPPFLAGS) -DPACKAGE=\\\"$(PACKAGE)\\\"
MSGFMT = msgfmt
XGETTEXT = xgettext
XGETTEXT_OPTS = --keyword=_ --keyword=P2_:1,1 --keyword=P3_:1,2 --language=C --add-comments
MSGMERGE = msgmerge

LDFLAGS =
LDFLAGS_shared = $(LDFLAGS) --shared
LDFLAGS_shared_LINUX = $(LDFLAGS) --shared
LDFLAGS_shared_SUN = $(LDFLAGS) -G
LDFLAGS_shared_HP = $(LDFLAGS) -b
LDFLAGS_lib = $(LDFLAGS_shared)
LDFLAGS_lib_LINUX = $(LDFLAGS_shared_LINUX) \
	-Wl,--soname,$(SHARED_LIB),--version-script,$(MAP_LIB)
LDFLAGS_lib_SUN = $(LDFLAGS_shared_SUN)
LDFLAGS_lib_HP = $(LDFLAGS_shared_HP)
LDFLAGS_lib_CYGWIN = $(LDFLAGS_shared) \
	-Wl,--out-implib=$(DEVEL_LIB_CYGWIN) \
	-Wl,--export-all-symbols \
	-Wl,--enable-auto-import
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
LDLIBS_pam_DARWIN = -lpam -lSystem

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
BINS = pwqgen pwqcheck pwqfilter
BINS_CYGWIN = $(BINS) $(SHARED_LIB_CYGWIN)
PROJ = $(SHARED_LIB) $(DEVEL_LIB) $(SHARED_PAM) $(BINS) $(PKGCONFIG)
OBJS_LIB = concat.o md4.o passwdqc_check.o passwdqc_filter.o passwdqc_load.o passwdqc_memzero.o passwdqc_parse.o passwdqc_random.o wordset_4k.o
OBJS_PAM = pam_passwdqc.o passwdqc_memzero.o
OBJS_GEN = pwqgen.o passwdqc_memzero.o
OBJS_CHECK = pwqcheck.o passwdqc_memzero.o
OBJS_FILTER = pwqfilter.o md4.o

default: all

all locales pam utils install install_lib install_locales install_pam install_utils uninstall remove remove_lib remove_locales remove_pam remove_utils:
	case "`uname -s`" in \
	Linux)	$(MAKE) CPPFLAGS_lib="$(CPPFLAGS_lib) -DHAVE_SHADOW" \
			LDFLAGS_lib="$(LDFLAGS_lib_LINUX)" \
			LDFLAGS_pam="$(LDFLAGS_pam_LINUX)" \
			LDLIBS_pam="$(LDLIBS_pam_LINUX)" \
			$@_wrapped;; \
	SunOS)	$(MAKE) -e CPPFLAGS_lib="$(CPPFLAGS_lib) -DHAVE_SHADOW" \
			LD_lib=ld \
			LDFLAGS_lib="$(LDFLAGS_lib_SUN)" \
			LDFLAGS_pam="$(LDFLAGS_pam_SUN)" \
			LDLIBS_pam="$(LDLIBS_pam_SUN)" \
			INSTALL="$(INSTALL_SUN)" \
			SHARED_LIBDIR="$(SHARED_LIBDIR_SUN)" \
			SECUREDIR="$(SECUREDIR_SUN)" \
			$@_wrapped;; \
	HP-UX)	$(MAKE) CPPFLAGS_lib="$(CPPFLAGS_lib) -DHAVE_SHADOW" \
			LD_lib=ld \
			LDFLAGS_lib="$(LDFLAGS_lib_HP)" \
			LDFLAGS_pam="$(LDFLAGS_pam_HP)" \
			LDLIBS_pam="$(LDLIBS_pam_HP)" \
			$@_wrapped;; \
	Darwin)	$(MAKE) \
			SHARED_LIB="$(SHARED_LIB_DARWIN)" \
			DEVEL_LIB="$(DEVEL_LIB_DARWIN)" \
			SECUREDIR="$(SECUREDIR_DARWIN)" \
			LDLIBS_pam="$(LDLIBS_pam_DARWIN)" \
			$@_wrapped;; \
    CYGWIN_NT*)	$(MAKE) CPPFLAGS_lib="$(CPPFLAGS_lib)" \
            SHARED_LIB="$(SHARED_LIB_CYGWIN)" \
            SHARED_LIBDIR="$(SHARED_LIBDIR_CYGWIN)" \
            DEVEL_LIB="$(DEVEL_LIB_CYGWIN)" \
            LDFLAGS_lib="$(LDFLAGS_lib_CYGWIN)" \
            BINS="$(BINS_CYGWIN)" \
            CYGWIN=true \
            $@_wrapped;; \
	*)	$(MAKE) $@_wrapped;; \
	esac

all_wrapped: pam_wrapped utils_wrapped $(PKGCONFIG)

pam_wrapped: $(SHARED_PAM)

utils_wrapped: $(BINS)

$(SHARED_LIB): $(OBJS_LIB) $(MAP_LIB)
	$(LD_lib) $(LDFLAGS_lib) $(OBJS_LIB) $(LDLIBS_lib) -o $(SHARED_LIB)

$(DEVEL_LIB): $(SHARED_LIB)
ifndef CYGWIN
	$(LN_s) $(SHARED_LIB) $(DEVEL_LIB)
endif

$(SHARED_PAM): $(OBJS_PAM) $(MAP_PAM) $(DEVEL_LIB)
	$(LD_lib) $(LDFLAGS_pam) $(OBJS_PAM) $(LDLIBS_pam) -L. -lpasswdqc -o $(SHARED_PAM)

pwqgen: $(OBJS_GEN) $(DEVEL_LIB)
	$(LD) $(LDFLAGS) $(OBJS_GEN) -L. -lpasswdqc -o $@

pwqcheck: $(OBJS_CHECK) $(DEVEL_LIB)
	$(LD) $(LDFLAGS) $(OBJS_CHECK) -L. -lpasswdqc -o $@

pwqfilter: $(OBJS_FILTER)
	$(LD) $(LDFLAGS) $(OBJS_FILTER) -o $@

pwqgen.o: pwqgen.c passwdqc.h
	$(CC) $(CPPFLAGS_bin) $(CFLAGS_bin) -c $*.c

pwqcheck.o: pwqcheck.c passwdqc.h
	$(CC) $(CPPFLAGS_bin) $(CFLAGS_bin) -c $*.c

pwqfilter.o: pwqfilter.c passwdqc_filter.h passwdqc.h
	$(CC) $(CPPFLAGS_bin) $(CFLAGS_bin) -c $*.c

.c.o:
	$(CC) $(CPPFLAGS_lib) $(CFLAGS_lib) -c $*.c

$(PKGCONFIG): $(PKGCONFIG).in
	sed -e "s|@VERSION@|$(VERSION)|g" $< > $@

concat.o: concat.h
pam_passwdqc.o: passwdqc.h pam_macros.h
passwdqc_check.o: passwdqc.h passwdqc_filter.h wordset_4k.h
passwdqc_filter.o: passwdqc.h passwdqc_filter.h
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
ifndef CYGWIN
	$(LN_s) $(SHARED_LIBDIR_REL)/$(SHARED_LIB) \
		$(DESTDIR)$(DEVEL_LIBDIR)/$(DEVEL_LIB)
else
	$(INSTALL) -m $(SHLIBMODE) $(DEVEL_LIB) $(DESTDIR)$(DEVEL_LIBDIR)/
endif

	$(MKDIR) $(DESTDIR)$(INCLUDEDIR)
	$(INSTALL) -m $(INCMODE) $(HEADER) $(DESTDIR)$(INCLUDEDIR)/

	$(MKDIR) $(DESTDIR)$(PKGCONFIGDIR)
	$(INSTALL) -m $(PKGCONFMODE) $(PKGCONFIG) $(DESTDIR)$(PKGCONFIGDIR)/

	$(MKDIR) $(DESTDIR)$(MANDIR)/man3
	$(INSTALL) -m $(MANMODE) $(MAN3) $(DESTDIR)$(MANDIR)/man3/

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

POFILES = $(LANGUAGES:%=po/%.po)
MOFILES = $(LANGUAGES:%=po/%.mo)
POTFILE_DEPS = pam_passwdqc.c passwdqc_check.c
POTFILE = po/$(PACKAGE).pot

$(POTFILE): $(POTFILE_DEPS)
	$(XGETTEXT) $(XGETTEXT_OPTS) -o $@-t $^ && mv $@-t $@

.SUFFIXES: .po .mo

.po.mo:
	$(MSGFMT) -c -o $@-t $< && mv $@-t $@

update_po: $(POTFILE) $(POFILES)
	for f in $(POFILES); do $(MSGMERGE) -U $$f $< || exit; done

update_mo: $(MOFILES)

locales_wrapped: update_mo

install_locales_wrapped:
	for lang in $(LANGUAGES); do \
		$(MKDIR) $(DESTDIR)$(LOCALEDIR)/$$lang/LC_MESSAGES && \
		$(INSTALL) -m $(LOCALEMODE) po/$$lang.mo \
			$(DESTDIR)$(LOCALEDIR)/$$lang/LC_MESSAGES/$(PACKAGE).mo || exit; \
	done

uninstall_wrapped remove_wrapped: remove_pam_wrapped remove_utils_wrapped remove_lib_wrapped remove_locales_wrapped

remove_pam_wrapped:
	$(RM) $(DESTDIR)$(MANDIR)/man8/$(MAN8)
	$(RM) $(DESTDIR)$(SECUREDIR)/$(SHARED_PAM)

remove_utils_wrapped:
	for f in $(MAN1); do $(RM) $(DESTDIR)$(MANDIR)/man1/$$f; done
	for f in $(BINS); do $(RM) $(DESTDIR)$(BINDIR)/$$f; done

remove_lib_wrapped:
	for f in $(MAN5); do $(RM) $(DESTDIR)$(MANDIR)/man5/$$f; done
	for f in $(MAN3); do $(RM) $(DESTDIR)$(MANDIR)/man3/$$f; done
	for f in $(HEADER); do $(RM) $(DESTDIR)$(INCLUDEDIR)/$$f; done
	for f in $(PKGCONFIG); do $(RM) $(DESTDIR)$(PKGCONFIGDIR)/$$f; done
	for f in $(DEVEL_LIB); do $(RM) $(DESTDIR)$(DEVEL_LIBDIR)/$$f; done
	for f in $(SHARED_LIB); do $(RM) $(DESTDIR)$(SHARED_LIBDIR)/$$f; done
	for f in $(CONFIGS); do $(RM) $(DESTDIR)$(CONFDIR)/$$f; done

remove_locales_wrapped:
	for f in $(LANGUAGES); do $(RM) $(DESTDIR)$(LOCALEDIR)/$$f/LC_MESSAGES/$(PACKAGE).mo; done

clean:
	$(RM) $(PROJ) $(POTFILE) $(MOFILES) *.o

.PHONY: all all_wrapped clean install install_lib install_locales install_pam install_utils \
	pam pam_wrapped uninstall remove remove_lib remove_pam remove_utils \
	utils utils_wrapped \
	update_mo update_po \
	locales locales_wrapped \
	install_wrapped install_lib_wrapped install_locales_wrapped install_pam_wrapped \
	install_utils_wrapped \
	remove_wrapped remove_lib_wrapped remove_locales_wrapped remove_pam_wrapped \
	remove_utils_wrapped
