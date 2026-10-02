# Changelog

All notable changes to RatUI since the last release are documented here. The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and RatUI follows
[Semantic Versioning](https://semver.org/). While the version is 0.x, minor
releases may contain breaking API changes.

## [0.1.1] - 2026-10-02

### Added

- `RatUI::Visit` for variant visitation through `CoreTraits` (`std::visit` by default).
- `RatUI::FindValue( map, key )`: returns a pointer to the mapped value or `nullptr`, so callers don't depend on `std::pair`'s `->second`.
- `OPTIMIZE_DEBUG` option for `ratui_require_dependency`, building a fetched dependency optimized in Debug while keeping it link-compatible.

### Changed

- All container and variant access inside RatUI now goes through `CoreTraits` and the generic container functions
  (`Visit` / `Holds` / `Get`, `Find`, `Size`, ...) instead of std member functions and helpers, so containers overridden in
  `RatUIContainerImpl.h` no longer need to mirror the std API.
- `HashMap` forwards its optional parameters to `HashMapImpl` instead of reading std's `::hasher` / `::key_equal` typedefs.
- msdfgen is now built optimized in Debug builds, making text atlas generation usable in Debug.

### Fixed

- The string view override macro is now `RATUI_OVERRIDE_STRING_VIEW_IMPL` (was `RATUI_STRING_VIEW_IMPL`),
  matching the other override macros; the docs now name the real override macros.

[0.1.1]: https://github.com/AsherFarag/RatUI/compare/v0.1.0...v0.1.1
