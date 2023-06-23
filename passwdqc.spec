Summary: A password/passphrase strength checking and policy enforcement toolset.
Name: passwdqc
Version: 2.0.3
Release: owl1
License: BSD-compatible
Group: System Environment/Base
URL: https://www.openwall.com/passwdqc/
Source: https://www.openwall.com/passwdqc/%name-%version.tar.gz
Provides: pam_passwdqc = %version-%release
Obsoletes: pam_passwdqc < %version-%release
BuildRequires: pam-devel
BuildRoot: /override/%name-%version

%description
passwdqc is a password/passphrase strength checking and policy
enforcement toolset, including a PAM module (pam_passwdqc), command-line
programs (pwqcheck, pwqfilter, and pwqgen), and a library (libpasswdqc).

pam_passwdqc is normally invoked on password changes by programs such as
passwd(1).  It is capable of checking password or passphrase strength,
enforcing a policy, and offering randomly-generated passphrases, with
all of these features being optional and easily (re-)configurable.

pwqcheck and pwqgen are standalone password/passphrase strength checking
and random passphrase generator programs, respectively, which are usable
from scripts.

The pwqfilter program searches, creates, or updates binary passphrase
filter files, which can also be used with pwqcheck and pam_passwdqc.

libpasswdqc is the underlying library, which may also be used from
third-party programs.

%package devel
Summary: Libraries and header files for building passwdqc-aware applications.
Group: Development/Libraries
Requires: %name = %version-%release

%description devel
This package contains development libraries and header files needed for
building passwdqc-aware applications.

%prep
%setup -q

%{expand:%%define optflags_lib %{?optflags_lib:%optflags_lib}%{!?optflags_lib:%optflags}}

%build
%__make \
	CPPFLAGS='-DLINUX_PAM' \
	CFLAGS_bin='-Wall -W %optflags' \
	CFLAGS_lib='-Wall -W -fPIC %optflags_lib'

%install
rm -rf %buildroot
%__make install DESTDIR=%buildroot MANDIR=%_mandir \
	SHARED_LIBDIR=/%_lib DEVEL_LIBDIR=%_libdir \
	SECUREDIR=/%_lib/security

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%defattr(-,root,root)
%doc CHANGES LICENSE README pwqcheck.php
%config(noreplace) /etc/passwdqc.conf
/%_lib/lib*.so*
%_bindir/*
/%_lib/security/pam_passwdqc.so
%_mandir/man[158]/*

%files devel
%defattr(-,root,root)
%_includedir/*.h
%_libdir/pkgconfig/passwdqc.pc
%_libdir/lib*.so
%_mandir/man3/*

%changelog
* Fri Jun 23 2023 Dmitry V. Levin <ldv-at-owl.openwall.com> 2.0.3-owl1
- wordset_4k: Move "enroll" to the multiple spellings list (by Solar Designer)
- Don't #include <endian.h> on macOS (by Solar Designer)
- pwqfilter: Allow --pre-hashed after --hash* (by Solar Designer)
- Add pkg-config file (by Egor Ignatov)
- Makefile: add Cygwin support (by Chad Dougherty)
- Remove non-existent symbols from the linker version script
to fix -Wl,--no-undefined-version (by Fangrui Song)
- pam_passwdqc: extend enforce=users to support chpasswd PAM service
in addition to traditionally supported passwd

* Sun Apr 04 2021 Solar Designer <solar-at-owl.openwall.com> 2.0.2-owl1
- Changes by Dmitry V. Levin:
  - pam_passwdqc: enhance formatting of auto-generated policy descriptions
  - Add libpasswdqc(3) manual page
  - Add manual page links for all functions documented in libpasswdqc(3)
  - Package section 3 manual pages into devel subpackage
  - LICENSE: mention the license of CI scripts (which are not packaged)
- Update CHANGES

* Wed Mar 10 2021 Solar Designer <solar-at-owl.openwall.com> 2.0.1-owl1
- Changes by Dmitry V. Levin:
  - pam_passwdqc: enhance auto-generated policy descriptions
  - Makefile: use CPPFLAGS and LDFLAGS consistently
  - Makefile: remove *.po dependence on passwdqc.pot
  - Remove generated passwdqc.pot from the repository
  - po/ru.po: regenerate using "make update_po"
  - po/ru.po: translate new messages added in 1.9.0+
- wordset_4k: Move "whisky" to the multiple spellings list
- Increase maximum size of randomly-generated passphrases to 136 bits
- Add CHANGES based on two latest release announcements, start to maintain it

* Wed Feb 17 2021 Solar Designer <solar-at-owl.openwall.com> 2.0.0-owl2
- Update the package description to include pwqfilter.

* Tue Feb 16 2021 Solar Designer <solar-at-owl.openwall.com> 2.0.0-owl1
- Introduce and use passwdqc_params_free().

* Fri Jan 29 2021 Solar Designer <solar-at-owl.openwall.com> 1.9.0-owl1
- Add support for external wordlist, denylist, and binary filter.
- passwdqc_random(): Obtain all of the random bytes before the loop.
- Merge changes needed for building with Visual Studio on Windows.

* Mon Jan 25 2021 Solar Designer <solar-at-owl.openwall.com> 1.5.0-owl1
- Updated the included wordlist to avoid some inappropriate words in randomly
generated passphrases while not removing any words from the "word-based" check,
and also to have plenty of extra words for subsequent removal of more words
that might be considered inappropriate from the initial 4096 that are used for
randomly generated passphrases.

* Mon Jan 25 2021 Solar Designer <solar-at-owl.openwall.com> 1.4.1-owl1
- Set default for "max" to 72 (was 40).
- Document "similar" in pwqcheck print_help() and man page.
- Drop the CVS Id tags (stale ones would be confusing with our move to git).

* Wed Dec 25 2019 Dmitry V. Levin <ldv-at-owl.openwall.com> 1.4.0-owl1
- Implemented i18n support in pam_passwdqc, contributed by Oleg Solovyov,
Andrey Cherepanov, and me.  The i18n support is off by default, it can be
enabled if Linux-PAM is built using --enable-nls configure option.
- Implemented audit support in pam_passwdqc, contributed by Oleg Solovyov
and me.  The audit support is off by default, it can be enabled if Linux-PAM
is built using --enable-audit configure option.

* Mon Dec 09 2019 Solar Designer <solar-at-owl.openwall.com> 1.3.2-owl1
- Define _DEFAULT_SOURCE for our use of crypt(3) on newer glibc.
The problem was identified and this change tested by Dmitry V. Levin.
- Clarified in the man pages that /etc/passwdqc.conf is not read unless this
suggested file location is specified with the config= option.
- Clarified the OpenBSD configuration example.
- Escape the minus sign in the OpenBSD configuration example to make the
manpage linter happy, patch by Jackson Doak via Unit 193:
https://www.openwall.com/lists/passwdqc-users/2019/04/16/1

* Wed Jul 20 2016 Solar Designer <solar-at-owl.openwall.com> 1.3.1-owl1
- With "non-unix", initialize the pw_dir field in fake_pw now that (since
passwdqc 1.1.3 in 2009) passwdqc_check.c uses that field.
Bug reported by Jim Paris via Debian: https://bugs.debian.org/831356
- Use size_t for variables holding strlen() return values.
- Cap "max" at 10000 (in case a config set it higher; the default remains 40).
- Check against the shortest allowed password length prior to checking against
the old password (this affects reporting when the old password is empty).
- For zeroization of sensitive data, use a wrapper around memset() called via
a function pointer to reduce the likelihood of a compiler optimizing those
calls out and to allow for overriding of this function with an OS-specific
"secure" memory zeroization function.
- In pwqgen, set stdout to non-buffered, and zeroize and free our own buffer
holding the generated password.

* Wed Apr 24 2013 Solar Designer <solar-at-owl.openwall.com> 1.3.0-owl1
- When checking is_simple() after discounting a common character sequence,
apply the (negative) bias even for the passphrase length check.  Previously,
we were not doing this because passphrases are normally built from words, and
the same code was being used for the check for dictionary words.
- Expanded the list of common character sequences.  Along with the change
above, this reduces the number of passing passwords for RockYou top 100k from
35 to 18, and for RockYou top 1M from 2333 to 2273 (all of these are with
passwdqc's default policy).
- Moved the common character sequences check to be made after the dictionary
words check, to avoid introducing more cases of misreporting.
- Added pwqcheck.php, a PHP wrapper function around the pwqcheck program.

* Tue Apr 23 2013 Solar Designer <solar-at-owl.openwall.com> 1.2.4-owl1
- In randomly generated passphrases: toggle case of the first character of each
word only if we wouldn't achieve sufficient entropy otherwise, use a trailing
separator if we achieve sufficient entropy even with the final word omitted
(in fact, we now enable the use of different separators in more cases for this
reason), use dashes rather than spaces to separate words when different
separator characters are not in use.
- Expanded the allowed size of randomly-generated passphrases in bits (now it's
24 to 85 in the tools, and 24 to 136 in the passwdqc_random() interface).

* Wed Aug 15 2012 Solar Designer <solar-at-owl.openwall.com> 1.2.3-owl1
- Handle possible NULL returns from crypt().
- Declared all pre-initialized arrays and structs as const.
- Added Darwin (Mac OS X) support to the Makefile, loosely based on a patch by
Ronald Ip (thanks!)

* Tue Jun 22 2010 Solar Designer <solar-at-owl.openwall.com> 1.2.2-owl1
- Introduced the GNU'ish "uninstall" make target name (a synonym for "remove").
- Makefile updates to make the "install" and "uninstall" targets with their
default settings friendlier to Solaris systems.
- Added a link to a wiki page with detailed Solaris-specific instructions to
the PLATFORMS file.

* Sat Mar 27 2010 Solar Designer <solar-at-owl.openwall.com> 1.2.1-owl1
- When matching against the reversed new password, always pass the original
non-reversed new password (possibly with a substring removed) into is_simple(),
but remove or check the correct substring in is_based() considering that the
matching is possibly being done against the reversed password.

* Tue Mar 16 2010 Solar Designer <solar-at-owl.openwall.com> 1.2.0-owl1
- New command-line options for pwqcheck: -1 and -2 for reading just 1 and
just 2 lines from stdin, respectively (instead of reading 3 lines, which is
the default), --multi for checking multiple passphrases at once (until EOF).
- With randomly-generated passphrases, encode more entropy per separator
character (by increasing the number of different separators from 8 to 16) and
per word (by altering the case of the first letter of each word), which
increases the default generated passphrase size from 42 to 47 bits.
- Substring matching has been enhanced to partially discount rather than fully
remove weak substrings, support leetspeak, and detect some common sequences of
characters (sequential digits, letters in alphabetical order, adjacent keys on
a QWERTY keyboard).
- Detect and allow passphrases with non-ASCII characters in the words.
- A number of optimizations have been made resulting in significant speedup
of passwdqc_check() on real-world passwords.
- Don't require %%optflags_lib such that the package can be built with
"rpmbuild -tb" on the tarball on non-Owl.

* Fri Oct 30 2009 Dmitry V. Levin <ldv-at-owl.openwall.com> 1.1.4-owl1
- Added const qualifier to all arguments of passwdqc_check() and
passwdqc_random().
- Implemented pwqcheck's stdin check for too long lines.
- Applied markup corrections to passwdqc.conf(5) and pwqcheck(1) for better
portability (by Kevin Steves and Jason McIntyre, with minor changes made
by Solar Designer).
- Changed use of mdoc's .Os macro to be consistent with other Openwall
Project's software (by Solar Designer).

* Wed Oct 21 2009 Dmitry V. Levin <ldv-at-owl.openwall.com> 1.1.3-owl1
- Eliminated insufficiently portable EXIT_FAILURE and EXIT_SUCCESS macros.
- In passwdqc_load.c, replaced redundant snprintf(3) with plain sprintf(3).
- Added pw_dir checks to passwdqc_check(), similar to already existing
pw_gecos checks.
- Dropped undocumented support for multiple options per config file line.
- Switched to a heavily cut-down BSD license.
- Added ldconfig calls to %%post and %%postun scripts.

* Sat Oct 17 2009 Solar Designer <solar-at-owl.openwall.com> 1.1.2-owl1
- In pwqcheck.c, replaced the uses of strsep(), which were insufficiently
portable, with code based on strchr().
- Corrected the linker invocations for Solaris (tested on Solaris 10) and
likely for HP-UX (untested).  We broke this between 1.0.5 and 1.1.0.
- Split the CFLAGS into two, separate for libraries (libpasswdqc, pam_passwdqc)
and binaries (the pwq* programs).
- In the Makefile, set umask 022 on mkdir's invoked by "make install".

* Thu Oct 15 2009 Dmitry V. Levin <ldv-at-owl.openwall.com> 1.1.1-owl1
- Relaxed license of pwqgen and pwqcheck manual pages.
- Ensure that pwqgen's exit status is zero only if generated passphrase
has been printed successfully.
- Changed pwqcheck to print "OK" line on success.
- Changed pwqcheck to print "Weak passphrase" diagnostics to stdout
instead of stderr.

* Sat Oct 10 2009 Solar Designer <solar-at-owl.openwall.com> 1.1.0-owl1
- Export passwdqc_params_load in libpasswdqc.
- Minor English grammar corrections to messages produced by pam_passwdqc.
- Minor documentation edits.
- Added/adjusted copyright statements and attributions to reflect Dmitry's
recent changes.

* Mon Sep 28 2009 Dmitry V. Levin <ldv-at-owl.openwall.com> unreleased
- Introduced libpasswdqc shared library.
- Implemented pwqgen and pwqcheck utilities.
- Implemented config= parameter support in libpasswdqc.
- Packaged /etc/passwdqc.conf file with default configuration.
- Added passwdqc.conf(5) manual page.

* Tue Feb 12 2008 Solar Designer <solar-at-owl.openwall.com> 1.0.5-owl1
- Replaced the separator characters with some of those defined by RFC 3986
as being safe within "userinfo" part of URLs without encoding.
- Reduced the default value for the N2 parameter to min=... (the minimum
length for passphrases) from 12 to 11.
- Corrected the potentially misleading description of N2 (Debian bug #310595).
- Applied minor grammar and style corrections to the documentation, a
pam_passwdqc message, and source code comments.

* Tue Apr 04 2006 Dmitry V. Levin <ldv-at-owl.openwall.com> 1.0.4-owl1
- Changed Makefile to pass list of libraries to linker after regular
object files, to fix build with -Wl,--as-needed.
- Corrected specfile to make it build on x86_64.

* Wed Aug 17 2005 Dmitry V. Levin <ldv-at-owl.openwall.com> 1.0.3-owl1
- Fixed potential memory leak in conversation wrapper.
- Restricted list of global symbols exported by the PAM module
to standard set of six pam_sm_* functions.

* Wed May 18 2005 Solar Designer <solar-at-owl.openwall.com> 1.0.2-owl1
- Fixed compiler warnings seen on FreeBSD 5.3.
- Updated the Makefile to not require editing on FreeBSD.
- Updated the FreeBSD-specific notes in PLATFORMS.

* Sun Mar 27 2005 Solar Designer <solar-at-owl.openwall.com> 1.0.1-owl1
- Further compiler warning fixes on LP64 platforms.

* Fri Mar 25 2005 Solar Designer <solar-at-owl.openwall.com> 1.0-owl1
- Corrected the source code to not break C strict aliasing rules.

* Wed Jan 26 2005 Solar Designer <solar-at-owl.openwall.com> 0.7.6-owl1
- Disallow unreasonable random= settings.
- Clarified the allowable bit sizes for randomly-generated passphrases and
the lack of relationship between passphrase= and random= options.

* Fri Oct 31 2003 Solar Designer <solar-at-owl.openwall.com> 0.7.5-owl1
- Assume invocation by root only if both the UID is 0 and the PAM service
name is "passwd"; this should solve changing expired passwords on Solaris
and HP-UX and make "enforce=users" safe.
- Produce proper English explanations for a wider variety of settings.
- Moved the "-c" out of CFLAGS, renamed FAKEROOT to DESTDIR.

* Sat Jun 21 2003 Solar Designer <solar-at-owl.openwall.com> 0.7.4-owl1
- Documented that "enforce=users" may not always work for services other
than the passwd command.
- Applied a patch to PLATFORMS from Mike Gerdts of GE Medical Systems
to reflect how Solaris 8 patch 108993-18 (or 108994-18 on x86) changes
Solaris 8's PAM implementation to look like Solaris 9.

* Mon Jun 02 2003 Solar Designer <solar-at-owl.openwall.com> 0.7.3.1-owl1
- Added URL.

* Thu Oct 31 2002 Solar Designer <solar-at-owl.openwall.com> 0.7.3-owl1
- When compiling with gcc, also link with gcc.
- Use $(MAKE) to invoke sub-makes.

* Fri Oct 04 2002 Solar Designer <solar-at-owl.openwall.com>
- Solaris 9 notes in PLATFORMS.

* Wed Sep 18 2002 Solar Designer <solar-at-owl.openwall.com>
- Build with Sun's C compiler cleanly, from Kevin Steves.
- Use install -c as that actually makes a difference on at least HP-UX
(otherwise install would possibly move files and not change the owner).

* Fri Sep 13 2002 Solar Designer <solar-at-owl.openwall.com>
- Have the same pam_passwdqc binary work for both trusted and non-trusted
HP-UX, from Kevin Steves.

* Fri Sep 06 2002 Solar Designer <solar-at-owl.openwall.com>
- Use bigcrypt() on HP-UX whenever necessary, from Kevin Steves of Atomic
Gears LLC.
- Moved the old password checking into a separate function.

* Wed Jul 31 2002 Solar Designer <solar-at-owl.openwall.com>
- Call it 0.6.

* Sat Jul 27 2002 Solar Designer <solar-at-owl.openwall.com>
- Documented that the man page is under the 3-clause BSD-style license.
- HP-UX 11 support.

* Tue Jul 23 2002 Solar Designer <solar-at-owl.openwall.com>
- Applied minor corrections to the man page and at the same time eliminated
unneeded/unimportant differences between it and the README.

* Sun Jul 21 2002 Solar Designer <solar-at-owl.openwall.com>
- 0.5.1: imported the pam_passwdqc(8) manual page back from FreeBSD.

* Tue Apr 16 2002 Solar Designer <solar-at-owl.openwall.com>
- 0.5: preliminary OpenPAM (FreeBSD-current) support in the code and related
code cleanups (thanks to Dag-Erling Smorgrav).

* Thu Feb 07 2002 Michail Litvak <mci-at-owl.openwall.com>
- Enforce our new spec file conventions.

* Sun Nov 04 2001 Solar Designer <solar-at-owl.openwall.com>
- Updated to 0.4:
- Added "ask_oldauthtok" and "check_oldauthtok" as needed for stacking with
the Solaris pam_unix;
- Permit for stacking of more than one instance of this module (no statics).

* Tue Feb 13 2001 Solar Designer <solar-at-owl.openwall.com>
- Install the module as mode 755.

* Tue Dec 19 2000 Solar Designer <solar-at-owl.openwall.com>
- Added "-Wall -fPIC" to the CFLAGS.

* Mon Oct 30 2000 Solar Designer <solar-at-owl.openwall.com>
- 0.3: portability fixes (this might build on non-Linux-PAM now).

* Fri Sep 22 2000 Solar Designer <solar-at-owl.openwall.com>
- 0.2: added "use_authtok", added README.

* Fri Aug 18 2000 Solar Designer <solar-at-owl.openwall.com>
- 0.1, "retry_wanted" bugfix.

* Sun Jul 02 2000 Solar Designer <solar-at-owl.openwall.com>
- Initial version (non-public).
