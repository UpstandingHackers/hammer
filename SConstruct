# -*- python -*-
import os
env = Environment()

env.MergeFlags("-std=gnu99 -Wall -Wextra -Werror -Wno-unused-parameter -Wno-attributes")

if not env['PLATFORM'] == 'darwin':
    env.MergeFlags("-lrt")

AddOption("--variant",
          dest="variant",
          nargs=1, type="choice",
          choices=["debug", "opt"],
          default="opt",
          action="store",
          help="Build variant (debug or opt)")

env['BUILDDIR'] = 'build/$VARIANT'

dbg = env.Clone(VARIANT='debug')
dbg.Append(CCFLAGS=['-g'])

opt = env.Clone(VARIANT='opt')
opt.Append(CCFLAGS="-O3")

if GetOption("variant") == 'debug':
    env = dbg
else:
    env = opt

if os.getenv("CC") == "clang" or env['PLATFORM'] == 'darwin':
    env.Replace(CC="clang",
                CXX="clang++")
Export('env')

env.SConscript(["src/SConscript"], variant_dir='build/$VARIANT/src')
env.SConscript(["examples/SConscript"], variant_dir='build/$VARIANT/examples')

env.Command('test', 'build/$VARIANT/src/test_suite', 'env LD_LIBRARY_PATH=build/$VARIANT/src $SOURCE')
