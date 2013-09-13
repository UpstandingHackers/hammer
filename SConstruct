# -*- python -*-
import os
env = Environment()

env.MergeFlags("-std=gnu99 -Wall -Wextra -Werror -Wno-unused-parameter -Wno-attributes -lrt")

AddOption("--variant",
          dest="variant",
          nargs=1, type="choice",
          choices=["debug", "opt"],
          default="opt",
          action="store",
          help="Build variant (debug or opt)")

AddOption("--coverage",
          dest="coverage",
          default=False,
          action="store_true",
          help="Build with coverage instrumentation")

env['BUILDDIR'] = 'build/$VARIANT'

dbg = env.Clone(VARIANT='debug')
dbg.Append(CCFLAGS=['-g'])

opt = env.Clone(VARIANT='opt')
opt.Append(CCFLAGS="-O3")

if GetOption("variant") == 'debug':
    env = dbg
else:
    env = opt

if GetOption("coverage"):
    env.Append(CFLAGS=["-fprofile-arcs", "-ftest-coverage"],
               CXXFLAGS=["-fprofile-arcs", "-ftest-coverage"],
               LDFLAGS=["-fprofile-arcs", "-ftest-coverage"],
               LIBS=['gcov'])

if os.getenv("CC") == "clang":
    env.Replace(CC="clang",
                CXX="clang++")
Export('env')

env.SConscript(["src/SConscript"], variant_dir='build/$VARIANT/src')
env.SConscript(["examples/SConscript"], variant_dir='build/$VARIANT/examples')

env.Command('test', 'build/$VARIANT/src/test_suite', 'env LD_LIBRARY_PATH=build/$VARIANT/src $SOURCE')
