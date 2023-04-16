# -*- python -*-

import os

PROGRAM='univ_tester'
DEPENDENCIES=['io', 'r8c']

DEP_SRC=[f"{d}/src" for d in DEPENDENCIES]
BASE_DIR = Dir('.').srcnode().abspath

baseEnv = Environment(
    ENV={'PATH' : os.environ['PATH']},
    AS='m32c-elf-as',
    CC='m32c-elf-gcc',
    CXX='m32c-elf-g++',
    CFLAGS='-std=c99',
    CXXFLAGS='-std=c++17',
    CPPFLAGS='-Wall -Werror -Wno-unused-variable -fno-exceptions -Os -mcpu=r8c',
    CPPPATH=['src'] + DEP_SRC,
    LINK='m32c-elf-gcc',
    LINKFLAGS=f"-mcpu=r8c -nostartfiles -Wl,-Map,build/{PROGRAM}.map -T r8c/m120an.ld -lsupc++",
)

env = baseEnv.Clone(LIBS=DEPENDENCIES, LIBPATH=DEPENDENCIES)
env.VariantDir('build', 'src', duplicate=0)

testEnv = Environment(
    ENV={'PATH' : os.environ['PATH']},
    LIBS=['pthread', 'libgtest', 'gcov'],
    CPPPATH=['src'] + DEP_SRC,
    CPPFLAGS='-coverage',
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

env.Depends(mot, elf)

lst = env.Command(
    f"build/{PROGRAM}.lst", elf, f"m32c-elf-objdump -h -S build/{PROGRAM}.elf > build/{PROGRAM}.lst"
)

env.Depends(lst, mot)

Default(lst)

testProg = testEnv.Program(f"build/test/{PROGRAM}", Glob('build/test/*.cpp'))

TEST_ONLY = os.getenv('TEST_ONLY')
_test = testEnv.Command(
    f"build/test/{PROGRAM}.log", testProg,
    f"build/test/{PROGRAM} " + ("" if TEST_ONLY is None else f"--gtest_filter={TEST_ONLY}") + f" | tee build/{PROGRAM}.log"
)

coverage = testEnv.Command(
    "build/test/coverage.info",
    Glob('build/test/*.gcda'),
    "lcov -c -d build/test -o build/test/coverage.info"
)

testEnv.Depends(coverage, _test)

coverage_html = testEnv.Command(
    "coverage",
    "build/test/coverage.info",
    "genhtml build/test/coverage.info -o coverage"
)
testEnv.Depends(coverage_html, coverage)

testEnv.Alias("test", coverage_html)

env.Clean(lst, ["build"])
testEnv.Clean(coverage_html, ["coverage"])

docs = testEnv.Command(f"{BASE_DIR}/html/index.html", [], f"doxygen {BASE_DIR}/Doxyfile")
testEnv.Clean(docs, "html")

testEnv.Alias("docs", docs)

os.environ['SKIP'] = "test docs"

r8c = env.SConscript('r8c/SConstruct')
env.Depends(elf, r8c)

io = env.SConscript('io/SConstruct')
env.Depends(elf, io)
