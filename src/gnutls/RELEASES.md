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
