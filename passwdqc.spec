# $Id: Owl/packages/passwdqc/passwdqc/passwdqc.spec,v 1.7 2001/02/13 00:36:01 solar Exp $

Summary: Pluggable password "quality check"
Name: pam_passwdqc
Version: 0.3
Release: 3owl
Copyright: relaxed BSD and (L)GPL-compatible
Group: System Environment/Base
Source: pam_passwdqc-%{version}.tar.gz
Buildroot: /var/rpm-buildroot/%{name}-%{version}

%description
pam_passwdqc is a simple password strength checking module for
PAM-aware password changing programs, such as passwd(1).  In addition
to checking regular passwords, it offers support for passphrases and
can provide randomly generated passwords.  All features are optional
and can be (re-)configured without rebuilding.

%prep
%setup -q

%build
make CFLAGS="-c -Wall -fPIC $RPM_OPT_FLAGS"

%install
rm -rf $RPM_BUILD_ROOT
make install FAKEROOT=$RPM_BUILD_ROOT
chmod 755 $RPM_BUILD_ROOT/lib/security/pam_passwdqc.so

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc LICENSE README
/lib/security/pam_passwdqc.so

%changelog
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
