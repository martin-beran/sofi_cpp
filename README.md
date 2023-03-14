[Doxygen]: index.html
[LICENSE]: LICENSE.md
[doc/intro]: doc/intro.md

# SOFI C++ library

SOFI (Subjects and Objects with Floating Integrity) security model
implementation as a C++ library.

## Author

Martin Beran

<martin@mber.cz>

## License

This software is available under the terms of BSD 2-Clause License, see
file [LICENSE.md][LICENSE].

## Introduction

For motivation and an introduction to the SOFI model, see
[a longer introductory text][doc/intro]. It contains esentially the
concatenation of the two Quora posts referenced below.

My older experiments regarding to the SOFI model:

- [FreeBSD MAC kernel module](https://github.com/martin-beran/mac_sofi)
- [Proof-of-concept implementation in Prolog](https://github.com/martin-beran/sofi_poc)
- Articles in my series about software engineering on Quora: [Secure or Secured Software?](https://softlyaboutsoftware.quora.com/Secure-or-Secured-Software) and [Secure or Secured Software? A Proposed Solution](https://softlyaboutsoftware.quora.com/Secure-or-Secured-Software-A-Proposed-Solution)

## Using the library

This is a header-only library. To use it, include
[src/soficpp/soficpp.hpp](soficpp/soficpp.hpp).

## Building and running tests

There is a CMake configuration to build and run tests:

    cmake -S . -B build -D CMAKE_BUILD_TYPE=Debug -D USE_LTO=OFF
    cmake --build build -v -j `nproc`
    cmake --build build -t test

## Documentation

There are some Markdown documents, for example, this text. If CMake finds
a program for converting Markdown to HTML (`markdown` or `markdown_py`), it
creates target `doc` and adds it to `all`. It creates HTML version of Markdown
documentation in the build directory.

The library and test programs are documented using Doxygen. If CMake finds
Doxygen, it creates target `doxygen` that generates HTML documentation into
subdirectory `html/` of the build directory, e.g.,
[build/html/index.html][Doxygen].
