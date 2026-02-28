Summary: A password/passphrase strength checking and policy enforcement toolset
Name: passwdqc
Version: 2.0.3
Release: 9%{?dist}
# Two manual pages (pam_passwdqc.8 and passwdqc.conf.5) are under the
# 3-clause BSD-style license as specified within the files themselves.
# The rest of the files in this package fall under the terms of
# the heavily cut-down "BSD license".
License: BSD-3-Clause
URL: https://www.openwall.com/%name/
Source0: https://www.openwall.com/%name/%name-%version.tar.gz
Source1: https://www.openwall.com/%name/%name-%version.tar.gz.sign
Requires: pam_%name = %version-%release
Requires: %name-utils = %version-%release
BuildRequires: make
BuildRequires: audit-libs-devel
BuildRequires: gcc
BuildRequires: gettext
BuildRequires: libxcrypt-devel
BuildRequires: pam-devel

%package -n lib%name
Summary: Passphrase quality checker shared library
Provides: %name-lib = %version-%release
Obsoletes: %name-lib < %version

%package -n lib%name-devel
Summary: Development files for building %name-aware applications
Requires: lib%name = %version-%release
Provides: %name-devel = %version-%release
Obsoletes: %name-devel < %version

%package -n pam_%name
Summary: Pluggable passphrase quality checker
Requires: lib%name = %version-%release

%package utils
Summary: Password quality checker utilities
Requires: lib%name = %version-%release

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

%description -n lib%name
The lib%name is a passphrase strength checking library.
In addition to checking regular passphrases, it offers support
for passphrases and can provide randomly generated passphrases.
All features are optional and can be (re-)configured without
rebuilding.

This package contains shared %name library.

%description -n lib%name-devel
The lib%name is a passphrase strength checking library.
In addition to checking regular passphrases, it offers support
for passphrases and can provide randomly generated passphrases.
All features are optional and can be (re-)configured without
rebuilding.

This package contains development files needed for building
%name-aware applications.

%description -n pam_%name
pam_%name is a passphrase strength checking module for
PAM-aware passphrase changing programs, such as passwd(1).
In addition to checking regular passphrases, it offers support
for passphrases and can provide randomly generated passphrases.
All features are optional and can be (re-)configured without
rebuilding.

%description utils
This package contains standalone utilities which are usable from scripts:
pwqcheck (a standalone passphrase strength checking program),
pwqgen (a standalone random passphrase generator program), and
pwqfilter (a standalone program that searches, creates, or updates
binary passphrase filter files).

%prep
%setup

%build
make %{?_smp_mflags} all locales \
	CPPFLAGS="-DENABLE_NLS=1 -DHAVE_LIBAUDIT=1 -DLINUX_PAM=1" \
	CFLAGS_lib="$RPM_OPT_FLAGS -W -DLINUX_PAM -fPIC" \
	CFLAGS_bin="$RPM_OPT_FLAGS -W" \
	#

%install
make install install_locales \
	CC=false LD=false \
	INSTALL='install -p' \
	DESTDIR="$RPM_BUILD_ROOT" \
	MANDIR=%_mandir \
	SHARED_LIBDIR=/%_lib \
	DEVEL_LIBDIR=%_libdir \
	SECUREDIR=/%_lib/security \
	#

%find_lang passwdqc

%ldconfig_scriptlets -n lib%name

%files

%files -n lib%name -f passwdqc.lang
%config(noreplace) %_sysconfdir/passwdqc.conf
/%_lib/lib*.so*
%_mandir/man5/*.5*
%doc LICENSE README *.php

%files -n lib%name-devel
%_includedir/*.h
%_libdir/pkgconfig/passwdqc.pc
%_libdir/lib*.so
%_mandir/man3/*.3*

%files -n pam_%name
/%_lib/security/*
%_mandir/man8/*.8*

%files utils
%_bindir/*
%_mandir/man1/*.1*

%changelog
* Fri Jan 16 2026 Fedora Release Engineering <releng@fedoraproject.org> - 2.0.3-9
- Rebuilt for https://fedoraproject.org/wiki/Fedora_44_Mass_Rebuild

* Thu Jul 24 2025 Fedora Release Engineering <releng@fedoraproject.org> - 2.0.3-8
- Rebuilt for https://fedoraproject.org/wiki/Fedora_43_Mass_Rebuild

* Sat Feb 01 2025 Björn Esser <besser82@fedoraproject.org> - 2.0.3-7
- Add explicit BR: libxcrypt-devel

* Fri Jan 17 2025 Fedora Release Engineering <releng@fedoraproject.org> - 2.0.3-6
- Rebuilt for https://fedoraproject.org/wiki/Fedora_42_Mass_Rebuild

* Thu Jul 18 2024 Fedora Release Engineering <releng@fedoraproject.org> - 2.0.3-5
- Rebuilt for https://fedoraproject.org/wiki/Fedora_41_Mass_Rebuild

* Thu Jan 25 2024 Fedora Release Engineering <releng@fedoraproject.org> - 2.0.3-4
- Rebuilt for https://fedoraproject.org/wiki/Fedora_40_Mass_Rebuild

* Sun Jan 21 2024 Fedora Release Engineering <releng@fedoraproject.org> - 2.0.3-3
- Rebuilt for https://fedoraproject.org/wiki/Fedora_40_Mass_Rebuild

* Thu Jul 20 2023 Fedora Release Engineering <releng@fedoraproject.org> - 2.0.3-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_39_Mass_Rebuild

* Sun Jun 25 2023 Dmitry V. Levin <ldv@altlinux.org> - 2.0.3-1
- 2.0.2 -> 2.0.3.

* Thu Jan 19 2023 Fedora Release Engineering <releng@fedoraproject.org> - 2.0.2-5
- Rebuilt for https://fedoraproject.org/wiki/Fedora_38_Mass_Rebuild

* Fri Jul 22 2022 Fedora Release Engineering <releng@fedoraproject.org> - 2.0.2-4
- Rebuilt for https://fedoraproject.org/wiki/Fedora_37_Mass_Rebuild

* Thu Jan 20 2022 Fedora Release Engineering <releng@fedoraproject.org> - 2.0.2-3
- Rebuilt for https://fedoraproject.org/wiki/Fedora_36_Mass_Rebuild

* Thu Jul 22 2021 Fedora Release Engineering <releng@fedoraproject.org> - 2.0.2-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_35_Mass_Rebuild

* Sun Apr 04 2021 Dmitry V. Levin <ldv@altlinux.org> - 2.0.2-1
- 1.4.0 -> 2.0.2.

* Tue Jan 26 2021 Fedora Release Engineering <releng@fedoraproject.org> - 1.4.0-3
- Rebuilt for https://fedoraproject.org/wiki/Fedora_34_Mass_Rebuild

* Tue Jul 28 2020 Fedora Release Engineering <releng@fedoraproject.org> - 1.4.0-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_33_Mass_Rebuild

* Mon Jun 08 2020 Dmitry V. Levin <ldv@altlinux.org> - 1.4.0-1
- 1.3.0 -> 1.4.0 (#1748553).
- Imported package from ALT Sisyphus.

* Wed Jan 29 2020 Fedora Release Engineering <releng@fedoraproject.org> - 1.3.0-16
- Rebuilt for https://fedoraproject.org/wiki/Fedora_32_Mass_Rebuild

* Fri Jul 26 2019 Fedora Release Engineering <releng@fedoraproject.org> - 1.3.0-15
- Rebuilt for https://fedoraproject.org/wiki/Fedora_31_Mass_Rebuild

* Fri Feb 01 2019 Fedora Release Engineering <releng@fedoraproject.org> - 1.3.0-14
- Rebuilt for https://fedoraproject.org/wiki/Fedora_30_Mass_Rebuild

* Mon Jan 14 2019 Björn Esser <besser82@fedoraproject.org> - 1.3.0-13
- Rebuilt for libcrypt.so.2 (#1666033)

* Fri Jul 13 2018 Fedora Release Engineering <releng@fedoraproject.org> - 1.3.0-12
- Rebuilt for https://fedoraproject.org/wiki/Fedora_29_Mass_Rebuild

* Thu Feb 08 2018 Fedora Release Engineering <releng@fedoraproject.org> - 1.3.0-11
- Rebuilt for https://fedoraproject.org/wiki/Fedora_28_Mass_Rebuild

* Sat Jan 20 2018 Björn Esser <besser82@fedoraproject.org> - 1.3.0-10
- Rebuilt for switch to libxcrypt

* Thu Aug 03 2017 Fedora Release Engineering <releng@fedoraproject.org> - 1.3.0-9
- Rebuilt for https://fedoraproject.org/wiki/Fedora_27_Binutils_Mass_Rebuild

* Thu Jul 27 2017 Fedora Release Engineering <releng@fedoraproject.org> - 1.3.0-8
- Rebuilt for https://fedoraproject.org/wiki/Fedora_27_Mass_Rebuild

* Sat Feb 11 2017 Fedora Release Engineering <releng@fedoraproject.org> - 1.3.0-7
- Rebuilt for https://fedoraproject.org/wiki/Fedora_26_Mass_Rebuild

* Thu Feb 04 2016 Fedora Release Engineering <releng@fedoraproject.org> - 1.3.0-6
- Rebuilt for https://fedoraproject.org/wiki/Fedora_24_Mass_Rebuild

* Thu Jun 18 2015 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.3.0-5
- Rebuilt for https://fedoraproject.org/wiki/Fedora_23_Mass_Rebuild

* Sat Feb 21 2015 Till Maas <opensource@till.name> - 1.3.0-4
- Rebuilt for Fedora 23 Change
  https://fedoraproject.org/wiki/Changes/Harden_all_packages_with_position-independent_code

* Sun Aug 17 2014 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.3.0-3
- Rebuilt for https://fedoraproject.org/wiki/Fedora_21_22_Mass_Rebuild

* Fri Jun 06 2014 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.3.0-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_21_Mass_Rebuild

* Mon Jul 29 2013 Avesh Agarwal <avagarwa@redhat.com> - 1.3.0-1
- New upstream release
- Fixes 815504

* Thu Feb 14 2013 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.2.2-5
- Rebuilt for https://fedoraproject.org/wiki/Fedora_19_Mass_Rebuild

* Fri Jul 20 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.2.2-4
- Rebuilt for https://fedoraproject.org/wiki/Fedora_18_Mass_Rebuild

* Fri Jan 13 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.2.2-3
- Rebuilt for https://fedoraproject.org/wiki/Fedora_17_Mass_Rebuild

* Tue Feb 08 2011 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.2.2-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_15_Mass_Rebuild

* Tue Oct 19 2010 Avesh Agarwal <avagarwa@redhat.com> - 1.2.2-1
- New upstream release with package name changed to passwdqc
- Added new command line utilities pwqcheck and pwqgen
- Addition of passwdqc.conf file in /etc
- Added libpasswdqc library and header files for buliding applications
- This is an initial release of passwdqc.

* Wed Aug 11 2010 Avesh Agarwal <avagarwa@redhat.com> - 1.0.5-7
- Fixed issues as per bz 226226, patch by Parag (panemade@gmail.com).
- Added explanation for extra C flags as per fedora guidelines.

* Tue Jan 12 2010 Avesh Agarwal <avagarwa@redhat.com> - 1.0.5-6
- Fixed following rpmlint errors on src rpm:
  pam_passwdqc.src: W: summary-ended-with-dot Pluggable
  password quality-control module.,
  pam_passwdqc.src:11: E: buildprereq-use pam-devel

* Tue Sep 29 2009 Avesh Agarwal <avagarwa@redhat.com> - 1.0.5-5
- Fixed an issue with spec file where "Release:" is not
  specified with "(?dist)". Without this, it gives problem
  when tagging across different fedora releases.

* Tue Sep 29 2009 Avesh Agarwal <avagarwa@redhat.com> - 1.0.5-4
- Patch for new configurable options(rhbz# 219201):
  disable first upper and last digit check, passwords
  prompts can be read from a file

* Sat Jul 25 2009 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.0.5-3
- Rebuilt for https://fedoraproject.org/wiki/Fedora_12_Mass_Rebuild

* Thu Feb 26 2009 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.0.5-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_11_Mass_Rebuild

* Thu Jun  5 2008 Tomas Mraz <tmraz@redhat.com> - 1.0.5-1
- upgrade to a latest upstream version

* Wed Feb 20 2008 Fedora Release Engineering <rel-eng@fedoraproject.org> - 1.0.4-5
- Autorebuild for GCC 4.3

* Wed Aug 22 2007 Tomas Mraz <tmraz@redhat.com> - 1.0.4-4
- clarify license even more

* Thu Aug 16 2007 Nalin Dahyabhai <nalin@redhat.com> - 1.0.4-3
- clarify license

* Sun Jul 29 2007 Nalin Dahyabhai <nalin@redhat.com> - 1.0.4-2
- set LDFLAGS_LINUX, not LDFLAGS, so that we don't strip the module before
  the debuginfo gets pulled out (#249963)

* Thu Jul 19 2007 Nalin Dahyabhai <nalin@redhat.com> - 1.0.4-1
- update to 1.0.4

* Wed Jul 12 2006 Jesse Keating <jkeating@redhat.com> - 1.0.2-1.2.2
- rebuild

* Fri Feb 10 2006 Jesse Keating <jkeating@redhat.com> - 1.0.2-1.2.1
- bump again for double-long bug on ppc(64)

* Tue Feb 07 2006 Jesse Keating <jkeating@redhat.com> - 1.0.2-1.2
- rebuilt for new gcc4.1 snapshot and glibc changes

* Fri Dec 09 2005 Jesse Keating <jkeating@redhat.com>
- rebuilt

* Fri Sep 16 2005 Nalin Dahyabhai <nalin@redhat.com> 1.0.2-1
- update to 1.0.2
- drop patch to use getpwnam_r() instead of getpwnam()

* Wed Mar 16 2005 Nalin Dahyabhai <nalin@redhat.com> 0.7.6-1
- update to 0.7.6, refines random= flag

* Mon Aug 16 2004 Nalin Dahyabhai <nalin@redhat.com> 0.7.5-2
- add %%clean stanza, pam-devel buildprereq

* Tue Apr 13 2004 Nalin Dahyabhai <nalin@redhat.com> 0.7.5-1
- bump release number to 1

* Tue Mar 30 2004 Nalin Dahyabhai <nalin@redhat.com> 0.7.5-0
- pull in Openwall package

* Fri Oct 31 2003 Solar Designer <solar@owl.openwall.com> 0.7.5-owl1
- Assume invocation by root only if both the UID is 0 and the PAM service
  name is "passwd"; this should solve changing expired passwords on Solaris
  and HP-UX and make "enforce=users" safe.
- Produce proper English explanations for a wider variety of settings.
- Moved the "-c" out of CFLAGS, renamed FAKEROOT to DESTDIR.

* Sat Jun 21 2003 Solar Designer <solar@owl.openwall.com> 0.7.4-owl1
- Documented that "enforce=users" may not always work for services other
  than the passwd command.
- Applied a patch to PLATFORMS from Mike Gerdts of GE Medical Systems
  to reflect how Solaris 8 patch 108993-18 (or 108994-18 on x86) changes
  Solaris 8's PAM implementation to look like Solaris 9.

* Mon Jun 02 2003 Solar Designer <solar@owl.openwall.com> 0.7.3.1-owl1
- Added URL.

* Thu Oct 31 2002 Solar Designer <solar@owl.openwall.com> 0.7.3-owl1
- When compiling with gcc, also link with gcc.
- Use $(MAKE) to invoke sub-makes.

* Fri Oct 04 2002 Solar Designer <solar@owl.openwall.com>
- Solaris 9 notes in PLATFORMS.

* Wed Sep 18 2002 Solar Designer <solar@owl.openwall.com>
- Build with Sun's C compiler cleanly, from Kevin Steves.
- Use install -c as that actually makes a difference on at least HP-UX
  (otherwise install would possibly move files and not change the owner).

* Fri Sep 13 2002 Solar Designer <solar@owl.openwall.com>
- Have the same pam_passwdqc binary work for both trusted and non-trusted
  HP-UX, from Kevin Steves.

* Fri Sep 06 2002 Solar Designer <solar@owl.openwall.com>
- Use bigcrypt() on HP-UX whenever necessary, from Kevin Steves of Atomic
  Gears LLC.
- Moved the old password checking into a separate function.

* Wed Jul 31 2002 Solar Designer <solar@owl.openwall.com>
- Call it 0.6.

* Sat Jul 27 2002 Solar Designer <solar@owl.openwall.com>
- Documented that the man page is under the 3-clause BSD-style license.
- HP-UX 11 support.

* Tue Jul 23 2002 Solar Designer <solar@owl.openwall.com>
- Applied minor corrections to the man page and at the same time eliminated
  unneeded/unimportant differences between it and the README.

* Sun Jul 21 2002 Solar Designer <solar@owl.openwall.com>
- 0.5.1: imported the pam_passwdqc(8) manual page back from FreeBSD.

* Tue Apr 16 2002 Solar Designer <solar@owl.openwall.com>
- 0.5: preliminary OpenPAM (FreeBSD-current) support in the code and related
code cleanups (thanks to Dag-Erling Smorgrav).

* Thu Feb 07 2002 Michail Litvak <mci@owl.openwall.com>
- Enforce our new spec file conventions.

* Sun Nov 04 2001 Solar Designer <solar@owl.openwall.com>
- Updated to 0.4:
- Added "ask_oldauthtok" and "check_oldauthtok" as needed for stacking with
the Solaris pam_unix;
- Permit for stacking of more than one instance of this module (no statics).

* Tue Feb 13 2001 Solar Designer <solar@owl.openwall.com>
- Install the module as mode 755.

* Tue Dec 19 2000 Solar Designer <solar@owl.openwall.com>
- Added "-Wall -fPIC" to the CFLAGS.

* Mon Oct 30 2000 Solar Designer <solar@owl.openwall.com>
- 0.3: portability fixes (this might build on non-Linux-PAM now).

* Fri Sep 22 2000 Solar Designer <solar@owl.openwall.com>
- 0.2: added "use_authtok", added README.

* Fri Aug 18 2000 Solar Designer <solar@owl.openwall.com>
- 0.1, "retry_wanted" bugfix.

* Sun Jul 02 2000 Solar Designer <solar@owl.openwall.com>
- Initial version (non-public).
