# -*- python -*-

import os

PROGRAM='univ_tester'

baseEnv = Environment(
    ENV = {'PATH' : os.environ['PATH']},
    AS='m32c-elf-as',
    CC='m32c-elf-gcc',
    CXX='m32c-elf-g++',
    CFLAGS='-std=gnu99',
    CXXFLAGS='-std=c++14',
    CPPFLAGS='-Wall -Werror -Wno-unused-variable -fno-exceptions -Os -mcpu=r8c',
    CPPPATH=['src', 'io/src', 'r8c/src'],
    LINK='m32c-elf-gcc',
    LINKFLAGS=f"-mcpu=r8c -nostartfiles -Wl,-Map,{PROGRAM}.map -T r8c/m120an.ld -lsupc++",
)

env = baseEnv.Clone(LIBS=['r8c', 'r8c_io'], LIBPATH=['io', 'r8c'])
env.VariantDir('build', 'src', duplicate=0)

testEnv = Environment(
    ENV = {'PATH' : os.environ['PATH']},
    LIBS=['pthread', 'libgtest'],
    CPPPATH=['src', 'io/src'],
)
testEnv.VariantDir('build', 'src', duplicate=0)

r8c_io = SConscript('io/SConstruct')
r8c = SConscript('r8c/SConstruct')

elf = env.Program(
    f"{PROGRAM}.elf", [
        'build/main.cpp',
    ],
)

Depends(elf, r8c_io)
Depends(elf, r8c)

testProg = testEnv.Program(
    PROGRAM, [
        'build/test/main_test.cpp',
    ]
)

mot = env.Command(
    f"{PROGRAM}.mot", elf, f"m32c-elf-objcopy --srec-forceS3 --srec-len 32 -O srec {PROGRAM}.elf {PROGRAM}.mot"
)

lst = env.Command(
    f"{PROGRAM}.lst", elf, f"m32c-elf-objdump -h -S {PROGRAM}.elf > {PROGRAM}.lst"
)

Depends(lst, mot)

TEST_ONLY = os.getenv('TEST_ONLY')
test = testEnv.Command(
    f"./{PROGRAM}.log", testProg,
    f"./{PROGRAM} " + ("" if TEST_ONLY is None else f"--gtest_filter={TEST_ONLY}") + f" | tee {PROGRAM}.log"
)
testEnv.AlwaysBuild(test)
Alias("test", f"{PROGRAM}.log")

Default(lst)
