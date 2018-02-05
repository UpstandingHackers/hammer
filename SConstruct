# -*- python -*-
import os
import os.path
import platform
import sys

default_install_dir='/usr/local'
if platform.system() == 'Windows':
    default_install_dir = 'build' # no obvious place for installation on Windows

vars = Variables(None, ARGUMENTS)
vars.Add(PathVariable('DESTDIR', 'Root directory to install in (useful for packaging scripts)', None, PathVariable.PathIsDirCreate))
vars.Add(PathVariable('prefix', 'Where to install in the FHS', default_install_dir, PathVariable.PathAccept))
vars.Add(ListVariable('bindings', 'Language bindings to build', 'none', ['cpp', 'dotnet', 'perl', 'php', 'python', 'ruby']))

tools = ['default', 'scanreplace']
if 'dotnet' in ARGUMENTS.get('bindings', []):
	tools.append('csharp/mono')

envvars = {'PATH' : os.environ['PATH']}
if 'PKG_CONFIG_PATH' in os.environ:
    envvars['PKG_CONFIG_PATH'] = os.environ['PKG_CONFIG_PATH']
if platform.system() == 'Windows':
    # from the scons FAQ (keywords: LNK1104 TEMPFILE), needed by link.exe
    envvars['TMP'] = os.environ['TMP']

env = Environment(ENV = envvars,
                  variables = vars,
                  tools=tools,
                  toolpath=['tools'])

if not 'bindings' in env:
    env['bindings'] = []

def calcInstallPath(*elements):
    path = os.path.abspath(os.path.join(*map(env.subst, elements)))
    if 'DESTDIR' in env:
        path = os.path.join(env['DESTDIR'], os.path.relpath(path, start='/'))
    return path

rel_prefix = not os.path.isabs(env['prefix'])
env['prefix'] = os.path.abspath(env['prefix'])
if 'DESTDIR' in env:
    env['DESTDIR'] = os.path.abspath(env['DESTDIR'])
    if rel_prefix:
        print >>sys.stderr, '--!!-- You used a relative prefix with a DESTDIR. This is probably not what you'
        print >>sys.stderr, '--!!-- you want; files will be installed in'
        print >>sys.stderr, '--!!--    %s' % (calcInstallPath('$prefix'),)


env['libpath'] = calcInstallPath('$prefix', 'lib')
env['incpath'] = calcInstallPath('$prefix', 'include', 'hammer')
env['parsersincpath'] = calcInstallPath('$prefix', 'include', 'hammer', 'parsers')
env['backendsincpath'] = calcInstallPath('$prefix', 'include', 'hammer', 'backends')
env['pkgconfigpath'] = calcInstallPath('$prefix', 'lib', 'pkgconfig')
env.ScanReplace('libhammer.pc.in')

AddOption('--variant',
          dest='variant',
          nargs=1, type='choice',
          choices=['debug', 'opt'],
          default='opt',
          action='store',
          help='Build variant (debug or opt)')

AddOption('--coverage',
          dest='coverage',
          default=False,
          action='store_true',
          help='Build with coverage instrumentation')

AddOption('--in-place',
          dest='in_place',
          default=False,
          action='store_true',
          help='Build in-place, rather than in the build/<variant> tree')

AddOption('--tests',
          dest='with_tests',
          default=env['PLATFORM'] != 'win32',
          action='store_true',
          help='Build tests')

env['CC'] = os.getenv('CC') or env['CC']
env['CXX'] = os.getenv('CXX') or env['CXX']

if os.getenv('CC') == 'clang' or (env['PLATFORM'] == 'darwin' and os.getenv('CC') == None):
    env.Replace(CC='clang',
                CXX='clang++')

# Language standard and warnings
if env['CC'] == 'cl':
    env.MergeFlags('-W3 -WX')
    env.Append(
        CPPDEFINES=[
            '_CRT_SECURE_NO_WARNINGS' # allow uses of sprintf
        ],
        CFLAGS=[
            '-wd4018', # 'expression' : signed/unsigned mismatch
            '-wd4244', # 'argument' : conversion from 'type1' to 'type2', possible loss of data
            '-wd4267', # 'var' : conversion from 'size_t' to 'type', possible loss of data
        ]
    )
else:
    env.MergeFlags('-std=gnu99 -Wall -Wextra -Werror -Wno-unused-parameter -Wno-attributes -Wno-unused-variable -Wno-unknown-warning-option')

# Linker options
if env['PLATFORM'] == 'darwin':
    env.Append(SHLINKFLAGS = '-install_name ' + env['libpath'] + '/${TARGET.file}')
elif platform.system() == 'OpenBSD':
    pass
elif env['PLATFORM'] == 'win32':
    # no extra lib needed
    pass
else:
    env.MergeFlags('-lrt')

if GetOption('coverage'):
    env.Append(CFLAGS=['--coverage'],
               CXXFLAGS=['--coverage'],
               LDFLAGS=['--coverage'])
    if env['CC'] == 'gcc':
        env.Append(LIBS=['gcov'])
    else:
        env.ParseConfig('llvm-config --ldflags')

dbg = env.Clone(VARIANT='debug')
if env['CC'] == 'cl':
    dbg.Append(CCFLAGS=['/Z7'])
else:
    dbg.Append(CCFLAGS=['-g'])

opt = env.Clone(VARIANT='opt')
if env['CC'] == 'cl':
    opt.Append(CCFLAGS=['/O2'])
else:
    opt.Append(CCFLAGS=['-O3'])

if GetOption('variant') == 'debug':
    env = dbg
else:
    env = opt

env['ENV'].update(x for x in os.environ.items() if x[0].startswith('CCC_'))

#rootpath = env['ROOTPATH'] = os.path.abspath('.')
#env.Append(CPPPATH=os.path.join('#', 'hammer'))

testruns = []

targets = ['$libpath',
           '$incpath',
           '$parsersincpath',
           '$backendsincpath',
           '$pkgconfigpath']

Export('env')
Export('testruns')
Export('targets')

if not GetOption('in_place'):
    env['BUILD_BASE'] = 'build/$VARIANT'
    lib = env.SConscript(['src/SConscript'], variant_dir='$BUILD_BASE/src')
    env.Alias('examples', env.SConscript(['examples/SConscript'], variant_dir='$BUILD_BASE/examples'))
else:
    env['BUILD_BASE'] = '.'
    lib = env.SConscript(['src/SConscript'])
    env.Alias(env.SConscript(['examples/SConscript']))

for testrun in testruns:
    env.Alias('test', testrun)

env.Alias('install', targets)
