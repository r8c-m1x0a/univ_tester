# -*- python -*-

import os

PROGRAM='univ_tester'
DEPENDENCIES=['io', 'r8c']

DEP_SRC=[f"{d}/src" for d in DEPENDENCIES]

commonEnv = Environment(
    ENV={'PATH' : os.environ['PATH']},
    CPPPATH=["src"] + DEP_SRC,
)

baseEnv = commonEnv.Clone(
    AS='m32c-elf-as',
    CC='m32c-elf-gcc',
    CXX='m32c-elf-g++',
    CFLAGS='-std=c99',
    CXXFLAGS='-std=c++17',
    CPPFLAGS='-Wall -Werror -Wno-unused-variable -fno-exceptions -Os -mcpu=r8c',
    LINK='m32c-elf-gcc',
    LINKFLAGS=f"-mcpu=r8c -nostartfiles -Wl,-Map,build/{PROGRAM}.map -T r8c/m120an.ld -lsupc++",
    LIBS=DEPENDENCIES,
    LIBPATH=DEPENDENCIES
)
env = baseEnv.Clone()
env.VariantDir("build", "src", duplicate=0)

testEnv = commonEnv.Clone(
    LIBS=['pthread', 'libgtest', 'gcov'],
    CPPFLAGS='-coverage',
)
testEnv.VariantDir("build/test", "src/test", duplicate=0)

elf = env.Program(
    f"build/{PROGRAM}.elf", [
        "build/main.cpp",
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

env.Alias("compile", lst)
Default(lst)

testProg = testEnv.Program(f"build/test/{PROGRAM}", Glob("build/test/*.cpp"))

TEST_ONLY = os.getenv('TEST_ONLY')
test = testEnv.Command(
    f"build/test/{PROGRAM}.log", testProg,
    f"build/test/{PROGRAM} " + ("" if TEST_ONLY is None else f"--gtest_filter={TEST_ONLY}") + f" | tee build/{PROGRAM}.log"
)

coverage = testEnv.Command(
    "build/test/coverage.info",
    Glob("build/test/*.gcda"),
    "lcov -c -d build/test -o build/test/coverage.info"
)
testEnv.Depends(coverage, test)

coverage_html = testEnv.Command(
    "coverage",
    "build/test/coverage.info",
    "genhtml build/test/coverage.info -o coverage"
)
testEnv.Depends(coverage_html, coverage)

testEnv.Alias("test", coverage_html)

env.Clean(lst, ["build"])
testEnv.Clean(coverage_html, ["coverage"])

docs = testEnv.Command("html/index.html", [], "doxygen Doxyfile")
testEnv.Clean(docs, "html")

testEnv.Alias("docs", docs)

os.environ['SKIP'] = "test docs"

io = baseEnv.SConscript("io/SConstruct")
env.Depends(elf, io)

r8c = baseEnv.SConscript("r8c/SConstruct")
env.Depends(elf, r8c)
