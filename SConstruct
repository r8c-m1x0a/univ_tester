# -*- python -*-

import os

PROGRAM='univ_tester'
DEPENDENCIES=['io', 'r8c']

BASE_DIR = Dir('.').srcnode().abspath
DEP_SRC=[f"{BASE_DIR}/{d}/src" for d in DEPENDENCIES]

commonEnv = Environment(
    ENV={'PATH' : os.environ['PATH']},
    CPPPATH=[f"{BASE_DIR}/src"] + DEP_SRC,
)

baseEnv = commonEnv.Clone(
    AS='m32c-elf-as',
    CC='m32c-elf-gcc',
    CXX='m32c-elf-g++',
    CFLAGS='-std=c99',
    CXXFLAGS='-std=c++17',
    CPPFLAGS='-Wall -Werror -Wno-unused-variable -fno-exceptions -Os -mcpu=r8c',
    LINK='m32c-elf-gcc',
    LINKFLAGS=f"-mcpu=r8c -nostartfiles -Wl,-Map,{BASE_DIR}/build/{PROGRAM}.map -T r8c/m120an.ld -lsupc++",
    LIBS=DEPENDENCIES,
    LIBPATH=DEPENDENCIES
)
env = baseEnv.Clone()
env.VariantDir(f"{BASE_DIR}/build", f"{BASE_DIR}/src", duplicate=0)

testEnv = commonEnv.Clone(
    LIBS=['pthread', 'libgtest', 'gcov'],
    CPPFLAGS='-coverage',
)
testEnv.VariantDir(f"{BASE_DIR}/build/test", f"{BASE_DIR}/src/test", duplicate=0)

elf = env.Program(
    f"{BASE_DIR}/build/{PROGRAM}.elf", [
        f"{BASE_DIR}/build/main.cpp",
    ],
)

mot = env.Command(
    f"{BASE_DIR}/build/{PROGRAM}.mot", elf, f"m32c-elf-objcopy --srec-forceS3 --srec-len 32 -O srec {BASE_DIR}/build/{PROGRAM}.elf {BASE_DIR}/build/{PROGRAM}.mot"
)
env.Depends(mot, elf)

lst = env.Command(
    f"{BASE_DIR}/build/{PROGRAM}.lst", elf, f"m32c-elf-objdump -h -S {BASE_DIR}/build/{PROGRAM}.elf > {BASE_DIR}/build/{PROGRAM}.lst"
)
env.Depends(lst, mot)

env.Alias("compile", lst)
Default(lst)

testProg = testEnv.Program(f"{BASE_DIR}/build/test/{PROGRAM}", Glob(f"{BASE_DIR}/build/test/*.cpp"))

TEST_ONLY = os.getenv('TEST_ONLY')
test = testEnv.Command(
    f"{BASE_DIR}/build/test/{PROGRAM}.log", testProg,
    f"{BASE_DIR}/build/test/{PROGRAM} " + ("" if TEST_ONLY is None else f"--gtest_filter={TEST_ONLY}") + f" | tee {BASE_DIR}/build/{PROGRAM}.log"
)

coverage = testEnv.Command(
    f"{BASE_DIR}/build/test/coverage.info",
    Glob(f"{BASE_DIR}/build/test/*.gcda"),
    f"lcov -c -d {BASE_DIR}/build/test -o {BASE_DIR}/build/test/coverage.info"
)
testEnv.Depends(coverage, test)

coverage_html = testEnv.Command(
    f"{BASE_DIR}/coverage",
    f"{BASE_DIR}/build/test/coverage.info",
    f"genhtml {BASE_DIR}/build/test/coverage.info -o {BASE_DIR}/coverage"
)
testEnv.Depends(coverage_html, coverage)

testEnv.Alias("test", coverage_html)

env.Clean(lst, [f"{BASE_DIR}/build"])
testEnv.Clean(coverage_html, [f"{BASE_DIR}/coverage"])

docs = testEnv.Command(f"{BASE_DIR}/html/index.html", [], f"doxygen {BASE_DIR}/Doxyfile")
testEnv.Clean(docs, f"{BASE_DIR}/html")

testEnv.Alias("docs", docs)

os.environ['SKIP'] = "test docs"

io = baseEnv.SConscript(f"{BASE_DIR}/io/SConstruct")
env.Depends(elf, io)

r8c = baseEnv.SConscript(f"{BASE_DIR}/r8c/SConstruct")
env.Depends(elf, r8c)
