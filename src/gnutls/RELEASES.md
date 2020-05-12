# Release policy

In the GnuTLS project there are at most two ongoing releases, one marked as
'stable' and one marked as 'next'. The former is always present and
supported, while the latter is introduced on the discretion of the
development team.

For both release branches all the rules in [CONTRIBUTING.md](CONTRIBUTING.md)
apply, but in principle the 'stable' release is more conservative in new features,
while the 'next' branch allows for mild behavioral changes. The 'stable' branch
always provides API and ABI compatibility, while the 'next' branch may on exceptional
cases change the API.


|Branch|Version|Release interval|
|:----:|:-----:|:--------------:|
|stable|3.6.x  |bi-monthly      |
|next  |-      |                |


# Release process

 0. Create a new 'milestone' for the next release and move all issues present in the
    current release milestone.
 1. Verification of release notes: ensure that release notes ([NEWS](NEWS)) exist
    for this release, and include all significant changes since last release.
 2. Update of release date in [NEWS](NEWS), and bump of version number in
    [configure.ac](configure.ac) as well as soname numbers in [m4/hooks.m4](m4/hooks.m4).
 3. make distcheck
 4. git tag -s $(VERSION). The 3.6.12 was including both the 3.6.12 and
    gnutls_3_6_12 tags, but it may make sense to only use the version from
    now on.
 5. git push && git push --tags
 6. make dist && gpg --sign --detach gnutls-$(VERSION).tar.xz
 7. scp gnutls-$(VERSION).tar.xz* trithemius.gnupg.org:/home/ftp/gcrypt/v3.6/
 8. Create and send announcement email based on previously sent email to the list and
    [NEWS](NEWS) file.
 9. Create a NEWS entry at [web-pages repository](https://gitlab.com/gnutls/web-pages/-/tree/master/news-entries),
    and/or [a security advisory entry](https://gitlab.com/gnutls/web-pages/-/tree/master/security-entries)
    if necessary. The NEWS entry is usually pointing to the announcement email.
    A commit auto-generates the [gnutls web site](https://gnutls.gitlab.io/web-pages/)
    which is mirrored twice a day by www.gnutls.org.
10. Use the @GnuTLS twitter account to announce the release.
11. Close the current release milestone.
