# Clar dependency

Source: https://github.com/clar-test/clar

Revision: `b53b06f625c4a98a038fcf62cce84baf52fe65df`

License: [ISC](COPYING). Vendored source files match the pinned revision without local patches. Repository format commands exclude `tests/native/vendor/`; its scoped Clang-format configuration also protects the C source during editor formatting. The project builds them locally and needs no network access to run tests.

## Dependency updates

Keep vendored source identical to an explicit upstream revision. Adopt fixes by updating the pinned revision and validating the suite. Keep project integration code outside the dependency.

## Known warning

This revision uses `codecs.open()`, which emits a deprecation warning on Python 3.14. Generation still completes with Python's normal warning settings. The warning remains visible pending an upstream fix. See the [Python deprecation reference](https://docs.python.org/3.14/library/codecs.html#codecs.open).
