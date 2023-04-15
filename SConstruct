# -*- python -*-

import os

PROGRAM='univ_tester'
DEPENDENCIES=['io', 'r8c']

baseEnv = Environment(
    ENV = {'PATH' : os.environ['PATH']},
    AS='m32c-elf-as',
    CC='m32c-elf-gcc',
    CXX='m32c-elf-g++',
    CFLAGS='-std=c99',
    CXXFLAGS='-std=c++17',
    CPPFLAGS='-Wall -Werror -Wno-unused-variable -fno-exceptions -Os -mcpu=r8c',
    CPPPATH=['src', 'io/src', 'r8c/src'],    
    LINK='m32c-elf-gcc',
    LINKFLAGS=f"-mcpu=r8c -nostartfiles -Wl,-Map,build/{PROGRAM}.map -T r8c/m120an.ld -lsupc++",
)

env = baseEnv.Clone(LIBS=['r8c', 'io'], LIBPATH=DEPENDENCIES)
env.VariantDir('build', 'src', duplicate=0)

testEnv = Environment(
    ENV = {'PATH' : os.environ['PATH']},
    LIBS=['pthread', 'libgtest', 'gcov'],
    CPPPATH=['src', 'io/src', 'r8c/src'],
    CPPFLAGS='-coverage'
)
testEnv.VariantDir('build', 'src', duplicate=0)

elf = env.Program(
    f"build/{PROGRAM}.elf", [
        'build/main.cpp',
    ],
)

mot = env.Command(
    f"build/{PROGRAM}.mot", elf, f"m32c-elf-objcopy --srec-forceS3 --srec-len 32 -O srec build/{PROGRAM}.elf build/{PROGRAM}.mot"
)

lst = env.Command(
    f"build/{PROGRAM}.lst", elf, f"m32c-elf-objdump -h -S build/{PROGRAM}.elf > build/{PROGRAM}.lst"
)

Depends(lst, mot)

testProg = testEnv.Program(f"build/test/{PROGRAM}", Glob('build/test/*.cpp'))

TEST_ONLY = os.getenv('TEST_ONLY')
test = testEnv.Command(
    f"build/test/{PROGRAM}.log", testProg,
    f"build/test/{PROGRAM} " + ("" if TEST_ONLY is None else f"--gtest_filter={TEST_ONLY}") + f" | tee build/{PROGRAM}.log"
)
testEnv.AlwaysBuild(test)
Alias("test", f"build/{PROGRAM}.log")

Default(lst)

coverage = testEnv.Command(
    "build/test/coverage.info",
    Glob('build/test/*.gcda'),
    "lcov -c -d build/test -o build/test/coverage.info"
)

coverage_html = testEnv.Command(
    "coverage",
    "build/test/coverage.info",
    "rm -rf html && genhtml build/test/coverage.info -o coverage"
)
Alias("test", [test, coverage_html])

Clean(lst, ["build", "coverage"])

r8c = SConscript('r8c/SConstruct')
Depends(elf, r8c)

io = SConscript('io/SConstruct')
Depends(elf, io)
