#
# Copyright (c) 2000-2003,2005 by Solar Designer. See LICENSE.
#

CC = gcc
LD = $(CC)
RM = rm -f
MKDIR = mkdir -p
INSTALL = install -c
CFLAGS = -Wall -fPIC -O2
LDFLAGS = --shared
LDFLAGS_LINUX = --shared -Wl,--version-script,$(MAP)
LDFLAGS_SUN = -G
LDFLAGS_HP = -b
LDLIBS = -lpam -lcrypt
LDLIBS_LINUX = -lpam -lcrypt
LDLIBS_SUN = -lpam -lcrypt
LDLIBS_HP = -lpam -lsec

# Uncomment this to use cc instead of gcc
#CC = cc
# Uncomment this to use Sun's C compiler flags
#CFLAGS = -KPIC -xO2
# Uncomment this to use HP's ANSI C compiler flags
#CFLAGS = -Ae +w1 +W 474,486,542 +z +O2

TITLE = pam_passwdqc
PAM_SO_SUFFIX =
LIBSHARED = $(TITLE).so$(PAM_SO_SUFFIX)
SHLIBMODE = 755
MAN8 = $(TITLE).8
MANMODE = 644
SECUREDIR = /lib/security
MANDIR = /usr/share/man
DESTDIR =

PROJ = $(LIBSHARED)
OBJS = pam_passwdqc.o passwdqc_check.o passwdqc_random.o wordset_4k.o
MAP = pam_passwdqc.map

all:
	case "`uname -s`" in \
	Linux)	$(MAKE) CFLAGS="$(CFLAGS) -DHAVE_SHADOW" \
			LDFLAGS="$(LDFLAGS_LINUX)" LDLIBS="$(LDLIBS_LINUX)" \
			$(PROJ);; \
	SunOS)	$(MAKE) CFLAGS="$(CFLAGS) -DHAVE_SHADOW" \
			LD=ld LDFLAGS="$(LDFLAGS_SUN)" LDLIBS="$(LDLIBS_SUN)" \
			$(PROJ);; \
	HP-UX)	$(MAKE) CFLAGS="$(CFLAGS) -DHAVE_SHADOW" \
			LD=ld LDFLAGS="$(LDFLAGS_HP)" LDLIBS="$(LDLIBS_HP)" \
			$(PROJ);; \
	*)	$(MAKE) $(PROJ);; \
	esac

$(LIBSHARED): $(OBJS) $(MAP)
	$(LD) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $(LIBSHARED)

.c.o:
	$(CC) $(CFLAGS) -c $*.c

pam_passwdqc.o: passwdqc.h pam_macros.h
passwdqc_check.o: passwdqc.h
passwdqc_random.o: passwdqc.h

install:
	$(MKDIR) $(DESTDIR)$(SECUREDIR)
	$(INSTALL) -m $(SHLIBMODE) $(LIBSHARED) $(DESTDIR)$(SECUREDIR)
	$(MKDIR) $(DESTDIR)$(MANDIR)/man8
	$(INSTALL) -m $(MANMODE) $(MAN8) $(DESTDIR)$(MANDIR)/man8/

remove:
	$(RM) $(DESTDIR)$(SECUREDIR)/$(LIBSHARED)
	$(RM) $(DESTDIR)$(MANDIR)/man8/$(MAN8)

clean:
	$(RM) $(PROJ) *.o
