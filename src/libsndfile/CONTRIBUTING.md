## Submitting Patches.

* Patches should pass all pre-commit hook tests.
* Patches should always be submitted via a either Github "pull request" or a
  via emailed patches created using "git format-patch".
* Patches for new features should include tests and documentation.
* Patches to fix bugs should either pass all tests, or modify the tests in some
  sane way.
* When a new feature is added for a particular file format and that feature
  makes sense for other formats, then it should also be implemented for one
  or two of the other formats.
