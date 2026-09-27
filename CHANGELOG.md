# Changelog

All notable changes to RatUI are documented here. The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and RatUI follows
[Semantic Versioning](https://semver.org/). While the version is 0.x, minor
releases may contain breaking API changes.

## [Unreleased]

## [0.1.0] - 2026-09-27

The first public release.

### Added

- **Scene and widgets:** `Scene` owns a widget tree and handles input dispatch, layout and rendering.
  Built-in widgets: `PanelWidget`, `ButtonWidget`, `TextWidget`, `InputTextWidget`, `SliderWidget` and `ScrollContainerWidget`.
- **Layout engine:** horizontal, vertical, overlay and grid layouts; content, fixed, percent and flex sizing;
  padding, margins, spacing, alignment, size constraints and anchored positioning.
- **Text:** MTSDF glyph atlas, HarfBuzz shaping, word wrapping with CJK line breaking rules,
  clip/ellipsis/fade overflow, outlines, drop shadows and per-glyph reveal. Editable text with selection and undo/redo.
- **Theming:** key-based colors, brushes (solid, texture, nine-slice), corner radii, fonts and text styles; built-in dark theme.
- **Input and navigation:** mouse, keyboard and gamepad events, focus, pointer capture, long-press,
  and directional navigation with focus scopes.
- **Animation:** easing curves and property interpolation.
- **Backends:** FreeType + HarfBuzz + msdfgen text backend, OpenGL 3.3 / WebGL2 renderer,
  and an experimental bgfx renderer. All are opt-in; the core has no third party dependencies.
- **Build:** CMake presets, automatic dependency fetching, install/export rules with a `RatUI::RatUI`
  package target, and `RATUI_VERSION_*` macros in `RatUI/Core/Config.h`.
- **Examples:** an SDL2 examples app that also builds to WebAssembly and is published as a live web demo.

[Unreleased]: https://github.com/AsherFarag/RatUI/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/AsherFarag/RatUI/releases/tag/v0.1.0
