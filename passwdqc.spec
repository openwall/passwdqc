Summary: Pluggable password "quality check"
Name: pam_passwdqc
Version: 0.0
Release: 1owl
Copyright: relaxed (L)GPL-compatible
Group: System Environment/Base
Source: pam_passwdqc-0.0.tar.gz
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
* Sun Jul  2 2000 Solar Designer <solar@false.com>
- initial version
