Hammer is a parsing library. Like many modern parsing libraries, it provides a parser combinator interface for writing grammars as inline domain-specific languages, but Hammer also provides a variety of parsing backends. It's also bit-oriented rather than character-oriented, making it ideal for parsing binary data such as images, network packets, audio, and executables.

Hammer is written in C, but will provide bindings for other languages. If you don't see a language you're interested in on the list, just ask.

Hammer currently builds under Linux and OS X. (Windows is coming.)

[![Build Status](https://travis-ci.org/UpstandingHackers/hammer.png)](https://travis-ci.org/UpstandingHackers/hammer)
Features
========
* Bit-oriented -- grammars can include single-bit flags or multi-bit constructs that span character boundaries, with no hassle
* Thread-safe, reentrant
* Benchmarking for parsing backends -- determine empirically which backend will be most time-efficient for your grammar
* Parsing backends:
  * Packrat parsing
  * LL(k) 
  * GLR 
  * LALR
  * Regular expressions 
* Language bindings: 
  * C++
  * Java (not currently building; give us a few days)
  * Python
  * Ruby
  * Perl
  * [Go](https://github.com/prevoty/hammer)
  * PHP
  * .NET 

Installing
==========
### Prerequisites
* SCons

### Optional Dependencies
* pkg-config (for `scons test`)
* glib-2.0 (>= 2.29) (for `scons test`)
* glib-2.0-dev (for `scons test`)
* swig (for Python/Perl/PHP bindings; Perl requires >= 2.0.8)
* python2.7-dev (for Python bindings)
* a JDK (for Java bindings)
* a working [phpenv](https://github.com/CHH/phpenv) configuration (for PHP bindings)
* Ruby >= 1.9.3 and bundler, for the Ruby bindings
* mono-devel and mono-mcs (>= 3.0.6) (for .NET bindings)
* nunit (for testing .NET bindings)

To build, type `scons`. To run the built-in test suite, type `scons test`. For a debug build, add `--variant=debug`.

To build bindings, pass a "bindings" argument to scons, e.g. `scons bindings=python`. `scons bindings=python test` will build Python bindings and run tests for both C and Python. `--variant=debug` is valid here too. You can build more than one set of bindings at a time; just separate them with commas, e.g. `scons bindings=python,perl`.

For Java, if jni.h and jni_md.h aren't already somewhere on your include path, prepend
`C_INCLUDE_PATH=/path/to/jdk/include` to that.

To make Hammer available system-wide, use `scons install`. This places include files in `/usr/local/include/hammer` 
and library files in `/usr/local/lib` by default; to install elsewhere, add a `prefix=<destination>` argument, e.g. 
`scons install prefix=$HOME`. A suitable `bindings=` argument will install bindings in whatever place your system thinks is appropriate.

Usage
=====
Just `#include <hammer/hammer.h>` (also `#include <hammer/glue.h>` if you plan to use any of the convenience macros) and link with `-lhammer`.

If you've installed Hammer system-wide, you can use `pkg-config` in the usual way.

To learn about hammer check
* the [user guide](https://github.com/UpstandingHackers/hammer/wiki/User-guide)
* [Hammer Primer](https://github.com/sergeybratus/HammerPrimer) (outdated in terms of code, but good to get the general thinking)
* [Try Hammer](https://github.com/sboesen/TryHammer)

Examples
========
The `examples/` directory contains some simple examples, currently including:
* base64
* DNS

Known Issues
============
The Python bindings only work with Python 2.7. SCons doesn't work with Python 3, and PyCapsule isn't available in 2.6 and below, so 2.7 is all you get. Sorry about that.

The requirement for SWIG >= 2.0.8 for Perl bindings is due to a [known bug](http://sourceforge.net/p/swig/patches/324/) in SWIG. [ppa:dns/irc](https://launchpad.net/~dns/+archive/irc) has backports of SWIG 2.0.8 for Ubuntu versions 10.04-12.10; you can also [build SWIG from source](http://www.swig.org/download.html).

The .NET bindings are for Mono 3.0.6 and greater. If you're on a Debian-based distro that only provides Mono 2 (e.g., Ubuntu 12.04), there are backports for [3.0.x](http://www.meebey.net/posts/mono_3.0_preview_debian_ubuntu_packages/), and a [3.2.x PPA](https://launchpad.net/~directhex/+archive/monoxide) maintained by the Mono team.

Community
=========
Please join us at `#hammer` on `irc.upstandinghackers.com` if you have any questions or just want to talk about parsing.

Contact
=======
You can also email us at <hammer@upstandinghackers.com>.
