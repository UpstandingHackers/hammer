#!/usr/bin/env python

"""
setup.py for Hammer bindings
"""
import os, os.path, sys
from distutils.core import setup, Extension

invoked = os.getcwd()
if (os.path.dirname(sys.argv[0]) != ''):
    os.chdir(os.path.dirname(sys.argv[0]))

setup(name="hammer",
      version="0.9.0",
      author="Upstanding Hackers, LLC",
      author_email="hammer@upstandinghackers.com",
      url="https://github.com/UpstandingHackers/hammer",
      description="""The Hammer parser combinator library""",
      ext_modules=[Extension('_hammer', ['hammer.i'],
                             swig_opts=['-DHAMMER_INTERNAL__NO_STDARG_H',
                                        '-I../../'],
                             define_macros=[('SWIG', None)],
                             depends=['allocator.h',
                                      'export.h',
                                      'glue.h',
                                      'hammer.h',
                                      'internal.h',],
                             extra_compile_args=['-fPIC',
                                                 '-std=gnu99',],
                             include_dirs=['../../'],
                             library_dirs=['../../'],
                             libraries=['hammer'],)],

      py_modules=['hammer'],
)

os.chdir(invoked)
