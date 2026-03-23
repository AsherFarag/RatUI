# RatUI

RatUI is a Retained-Mode Graphical User Interface library built for C++20.
It's designed to work around your codebase rather than the other way around.

## Features

Hopefully some soon.

## Repository Structure

 ```bash
RatUI
 ├───Examples # Example apps using RatUI
 ├───Include/RatUI # Public API (*Note: This is all you need to use this library)
 ├───Scripts # Build and utility scripts (*Note: Only needed for examples and tests)
 └───Tests # Unit and integration tests
 ```

## Requirements
RatUI is (currently?) a C++20, header-only library, so all you have to do is copy and paste Include/RatUI/ into your project.

TODO: Implement default backends and mention them here

For building and running the examples, you'll need the following:
- C++20‑compatible compiler (GCC, Clang, MSVC)
- [CMake](https://cmake.org/) 3.16+

## Building Examples from Source

1. **Clone the repository:**

   ```bash
   git clone https://github.com/AsherFarag/RatUI.git
   cd RatUI
   ```
2. **Configure and build with CMake:**

   ```bash
   mkdir build
   cd build
   cmake ..
   cmake --build .
   ```
3. **Run examples:**
   
   After building, you can run any example executables generated in the build folder.

# Contributing

TODO:
...

# License

RatUI is licensed under the **MIT License** - see the [LICENSE](https://github.com/AsherFarag/RatUI/blob/main/LICENSE) file for details.
