# $Id: Owl/packages/passwdqc/passwdqc/passwdqc.spec,v 1.2 2000/08/17 23:03:57 solar Exp $

Summary: Pluggable password "quality check"
Name: pam_passwdqc
Version: 0.1
Release: 1owl
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
make CFLAGS="-c $RPM_OPT_FLAGS"

%install
rm -rf $RPM_BUILD_ROOT
make install FAKEROOT=$RPM_BUILD_ROOT

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc LICENSE
/lib/security/pam_passwdqc.so

%changelog
* Fri Aug 18 2000 Solar Designer <solar@owl.openwall.com>
- 0.1, "retry_wanted" bugfix.

* Sun Jul 02 2000 Solar Designer <solar@owl.openwall.com>
- Initial version (non-public).
