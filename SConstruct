# -*- python -*-

import os

PROGRAM='uart_sample',

VariantDir('build', ['src', 'test', 'common', 'M120AN'], duplicate=0)

env = Environment(
    ENV = {'PATH' : os.environ['PATH']},
    AS='m32c-elf-as',
    CC='m32c-elf-gcc',
    CXX='m32c-elf-g++',
    CFLAGS='-std=gnu99',
    CXXFLAGS='-std=c++14',
    CPPFLAGS='-Wall -Werror -Wno-unused-variable -fno-exceptions -Os -mcpu=r8c',
    CPPPATH=['.', 'src'],
    LINK='m32c-elf-gcc',
    LINKFLAGS='-mcpu=r8c -nostartfiles -Wl,-Map,${PROGRAM}.map -T src/M120AN/m120an.ld -lsupc++',
)

testEnv = Environment(
    ENV = {'PATH' : os.environ['PATH']},
)

elf = env.Program(
    "${PROGRAM}.elf", [
        'build/main.cpp',
        'build/common/vect.c',
        'build/common/init.c',
        'build/common/start.s',
    ],
)

mot = env.Command(
    "${PROGRAM}.mot", elf, "m32c-elf-objcopy --srec-forceS3 --srec-len 32 -O srec ${PROGRAM}.elf ${PROGRAM}.mot"
)

lst = env.Command(
    "${PROGRAM}.lst", elf, "m32c-elf-objdump -h -S ${PROGRAM}.elf > ${PROGRAM}.lst"
)

Default([mot, lst])
