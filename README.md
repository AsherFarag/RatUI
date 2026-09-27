# <img src="Logo_SVG.svg" alt="RatUI Logo" style="width: 75px"> RatUI 

RatUI is a Retained-Mode Graphical User Interface library built for C++20.
It's designed for games and aims to integrate into your codebase rather than the other way around.

**[▶ Try Me](https://asherfarag.github.io/RatUI/)** — the examples running live in your browser (WebGL2).

> **Early days:** RatUI is pre-1.0. Expect breaking API changes between minor versions (0.1 → 0.2); see the [CHANGELOG](CHANGELOG.md).

---
![RatUI_Sandbox_LeObF4pG9w](https://github.com/user-attachments/assets/c17694b1-c2f8-42b1-94c2-398375ee9c72)
---
<img width="3840" height="1801" alt="image" src="https://github.com/user-attachments/assets/343a12e9-2f6d-47dd-bfca-7d51c0a73c91" />

## Features

* **Retained-mode widgets:** panels, buttons, text, text input, sliders and scroll containers, with a `Scene` that owns them and handles input, layout and drawing.
* **Layout engine:** horizontal, vertical, overlay and grid layouts, with content, fixed, percent and flex sizing, padding, margins, spacing, size constraints and anchored positioning.
* **Text:** MTSDF glyphs that stay sharp at any size, HarfBuzz shaping, word wrapping (including CJK line breaking rules), clip/ellipsis/fade overflow, outlines, drop shadows and per-glyph reveal for dialogue.
* **Theming:** colors, brushes, corner radii, fonts and text styles are looked up by key, so swapping a theme restyles the whole scene live.
* **Input and navigation:** mouse, keyboard and gamepad input; focus scopes and directional navigation for controller-driven menus.
* **Animation:** easing curves and interpolation for animating widget properties.
* **Backend-agnostic core:** the core has no third party dependencies. FreeType/HarfBuzz text and an OpenGL 3.3 / WebGL2 renderer are opt-in backends, and you can plug in your own through `IRenderer` and `ITextMetrics`.
* **Bring your own types:** math and container types (`String`, `Array`, ...) are user overridable and default to the STL.
* **Runs on the web:** the examples build to WebAssembly with Emscripten.

## Repository Structure

 ```bash
RatUI
 ├───Examples      # Example apps using RatUI
 ├───Include/RatUI # Public API
 ├───Source/RatUI  # Library sources
 ├───Scripts       # Build and utility scripts
 ├───cmake         # CMake helper modules and the package config template
 └───Tests         # Unit tests, plus a find_package() consumer in Tests/Package
 ```

## Requirements

**For examples and tests:**
- C++20‑compatible compiler (GCC, Clang, MSVC)
- [CMake](https://cmake.org/) 3.16+ (3.21+ to use the bundled `CMakePresets.json`)

## Example

A button with a label, using the OpenGL and FreeType backends. RatUI doesn't create windows or read input
itself: you give it an OpenGL context and translate your platform's events into `InputEvent`s.
[`Examples/Application`](Examples/Application/Application.cpp) is a complete SDL2 host to copy from.

```cpp
#include <RatUI/RatUI.h>
#include <RatUI/Backends/FreeType/FontLoader.h>
#include <RatUI/Backends/OpenGL/OpenGLRenderer.h>
#include <RatUI/Widget/ButtonWidget.h>
#include <RatUI/Widget/PanelWidget.h>
#include <RatUI/Widget/TextWidget.h>
#include <cstdio>

// Fonts are grouped into families and requested by (family, weight, style), CSS-style.
// Bold / italic are synthesized only when the family has no matching face.
RatUI::FontLibrary fonts;
RatUI::FreeType::FontLoader loader;

auto ui = fonts.RegisterFamily( "Inter"_id );
fonts.AddFaceToFamily( ui, loader.LoadFromFile( "Inter-Regular.ttf" ), { EFontWeight::Regular } );
fonts.AddFaceToFamily( ui, loader.LoadFromFile( "Inter-Bold.ttf" ),    { EFontWeight::Bold } );

// Pixel-art fonts: native grid auto-detected, drawn at whole-number scales, never blurry.
auto pixel = fonts.RegisterFamily( "PixelFont"_id );
fonts.AddFaceToFamily( pixel, loader.LoadFromFile( "PixelFont.ttf", { .Mode = EGlyphRenderMode::Pixel } ) );

auto textMetrics = RatUI::TextMetrics{ fonts };
auto atlas       = RatUI::GlyphAtlas{ renderer, fonts };
auto drawList    = RatUI::DrawList{ atlas };

auto scene = RatUI::Scene{};
scene.TextMetrics = &textMetrics;

scene.CreateRootWidget<RatUI::TextWidget>( "HP 42", RatUI::TextLayoutStyle{ .Family = ui, .Size = 18_u } );

// TODO Finish this example
```

`OpenGLRenderer.h` includes `<GL/glew.h>` by default; define `RATUI_OPENGL_INCLUDE` (e.g. `-DRATUI_OPENGL_INCLUDE=<glad/gl.h>`) to use a different loader.

## Building from Source

Everything is opt-in. A plain configure builds only the header-light core
library and needs no third party dependencies at all:

```bash
git clone https://github.com/AsherFarag/RatUI.git
cd RatUI
cmake -B build -S .
cmake --build build
```

Turn on what you actually need:

```bash
cmake -B build -S . -DRATUI_BUILD_TESTS=ON -DRATUI_BUILD_EXAMPLES=ON
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

Or use one of the bundled presets (`lib`, `tests`, `examples`, `dev`, `release`):

```bash
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
```

### Options

| Option | Default | Description |
| --- | --- | --- |
| `RATUI_FETCH_DEPENDENCIES` | `ON` (standalone) | Download and build any dependency `find_package()` cannot locate |
| `RATUI_BACKEND_FREETYPE` | `OFF` | FreeType font loading (FreeType + HarfBuzz + msdfgen): TTF / OTF faces in MTSDF, Raster or Pixel mode |
| `RATUI_BACKEND_OPENGL` | `OFF` | OpenGL renderer backend (GLEW + OpenGL) |
| `RATUI_BACKEND_BGFX` | `OFF` | bgfx renderer backend (**experimental**: not built in CI yet) |
| `RATUI_BUILD_TESTS` | `OFF` | Build the Catch2 test suite |
| `RATUI_BUILD_EXAMPLES` | `OFF` | Build the examples app (enables the OpenGL backend if no renderer is selected) |
| `RATUI_ENABLE_ASSERTS` | `ON` | Enable RatUI runtime assertions |
| `RATUI_INSTALL` | `ON` (standalone) | Generate install/export rules |

Enabling a backend defines a matching macro on the `RatUI` target
(`RATUI_BACKEND_FREETYPE=1`, `RATUI_BACKEND_OPENGL=1`, `RATUI_BACKEND_BGFX=1`),
so your code can check which ones are available.

### Dependencies

Every dependency is resolved the same way:

1. Use the target if your project already defines it.
2. Otherwise `find_package()` it (system, vcpkg, Conan, ...).
3. Otherwise, if `RATUI_FETCH_DEPENDENCIES=ON`, download and build it.
4. Otherwise fail with a message explaining what is missing.

The fetched versions are pinned in cache variables (`RATUI_FREETYPE_TAG`,
`RATUI_HARFBUZZ_TAG`, `RATUI_MSDFGEN_TAG`, `RATUI_SDL2_TAG`, `RATUI_CATCH2_TAG`,
`RATUI_BGFX_TAG`, `RATUI_GLEW_URL`) and can be overridden on the command line.

> Install rules are skipped automatically when a dependency was fetched, because
> a fetched dependency cannot be exported. Install the dependencies properly to
> produce an installable RatUI package.

### Web (WebAssembly)

The `web` preset builds the examples app with [Emscripten](https://emscripten.org/docs/getting_started/downloads.html)
(needs emsdk activated and Ninja). CI publishes it to GitHub Pages on every push to `main`.

```bash
cmake --preset web
cmake --build --preset web --target RatUI_Examples
python -m http.server -d build/web/Examples   # open RatUI_Examples.html
```

### Using RatUI in your project

With `FetchContent`:

```cmake
include(FetchContent)
FetchContent_Declare(RatUI
    GIT_REPOSITORY https://github.com/AsherFarag/RatUI.git
    GIT_TAG        v0.1.0)
set(RATUI_BACKEND_OPENGL ON)
set(RATUI_FETCH_DEPENDENCIES ON)
FetchContent_MakeAvailable(RatUI)

target_link_libraries(MyApp PRIVATE RatUI::RatUI)
```

Or against an installed copy:

```cmake
find_package(RatUI REQUIRED)
target_link_libraries(MyApp PRIVATE RatUI::RatUI)
```

## Contributing

Contributions are welcome.

For now:
* Open an issue to discuss changes or ideas
* Keep code consistent with the existing style
* Add and ensure tests (if applicable) pass

More detailed guidelines coming soon.

## License

RatUI is licensed under the **MIT License** - see the [LICENSE](https://github.com/AsherFarag/RatUI/blob/main/LICENSE) file for details.
