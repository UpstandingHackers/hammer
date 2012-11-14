Hammer is a parsing library. Like many modern parsing libraries, it provides a parser combinator interface for writing grammars as inline domain-specific languages, but Hammer also provides a variety of parsing backends. It's also bit-oriented rather than character-oriented, making it ideal for parsing binary data such as images, network packets, audio, and executables.

Hammer is written in C, but will provide bindings for other languages. If you don't see a language you're interested in on the list, just ask.

Hammer currently builds under Linux. (Windows and OSX are coming.)

Features
========
* Bit-oriented -- grammars can include single-bit flags or multi-bit constructs that span character boundaries, with no hassle
* Thread-safe, reentrant
* Benchmarking for parsing backends -- determine empirically which backend will be most time-efficient for your grammar
* Parsing backends:
  * Packrat parsing
  * LL(k) (not yet implemented)
  * GLR (not yet implemented)
  * LALR(8) (not yet implemented)
  * Regular expressions (not yet implemented)
* Language bindings: (not yet implemented)
  * C++
  * Java
  * Python
  * Ruby
  * Perl
  * Go
  * PHP
  * .NET

Installing
==========
### Prerequisites
* make

### Optional Dependencies
* pkg-config (for `make test`)
* glib-2.0 (>= 2.29) (for `make test`)
* glib-2.0-dev (for `make test`)

To install, type `make`. To run the built-in test suite, type `make test`.

There is not currently a `make install` target; to make Hammer available system-wide, copy `libhammer.a` to `/usr/lib/` (or `/usr/local/lib/`, or wherever ld will find it) and `hammer.h` to `/usr/include/`. 

Usage
=====
Just `#include <hammer.h>` and link with `-lhammer`.

Examples
========
The `examples/` directory contains some simple examples, currently including:
* base64
* DNS
