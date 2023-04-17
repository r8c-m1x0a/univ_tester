# -*- python -*-

import os

NAME='univ_tester'
DEPENDENCIES=['io', 'r8c']

DEP_SRC=[f"{d}/src/main" for d in DEPENDENCIES]
DEP_LIB=[f"{d}/build/main" for d in DEPENDENCIES]

commonEnv = Environment(
    ENV={'PATH' : os.environ['PATH']},
    CPPPATH=["src/main"] + DEP_SRC,
)

baseEnv = commonEnv.Clone(
    AS='m32c-elf-as',
    CC='m32c-elf-gcc',
    CXX='m32c-elf-g++',
    CFLAGS='-std=c99',
    CXXFLAGS='-std=c++17',
    CPPFLAGS='-Wall -Werror -Wno-unused-variable -fno-exceptions -Os -mcpu=r8c',
    LINK='m32c-elf-gcc',
    LINKFLAGS=f"-mcpu=r8c -nostartfiles -Wl,-Map,build/main/{NAME}.map -T r8c/m120an.ld -lsupc++",
    LIBS=DEPENDENCIES,
    LIBPATH=DEP_LIB
)
env = baseEnv.Clone()
env.VariantDir("build/main", "src/main", duplicate=0)

testEnv = commonEnv.Clone(
    LIBS=['pthread', 'libgtest', 'gcov'],
    CPPFLAGS='-coverage',
)
testEnv.VariantDir("build/test", "src/test", duplicate=0)

elf = env.Program(
    f"build/main/{NAME}.elf", [
        [Glob("build/main/*.cpp"), Glob("build/main/*.c"), Glob("build/main/*.cc")],
    ],
)

mot = env.Command(
    f"build/main/{NAME}.mot", elf, f"m32c-elf-objcopy --srec-forceS3 --srec-len 32 -O srec build/main/{NAME}.elf build/main/{NAME}.mot"
)
env.Depends(mot, elf)

lst = env.Command(
    f"build/main/{NAME}.lst", elf, f"m32c-elf-objdump -h -S build/main/{NAME}.elf > build/main/{NAME}.lst"
)
env.Depends(lst, mot)

env.Alias("compile", [lst, mot])
env.Clean("compile", ["build/main"])
Default(lst)

testProg = testEnv.Program(f"build/test/{NAME}", [Glob("build/test/*.cpp"), Glob("build/test/*.c"), Glob("build/test/*.cc")])

TEST_ONLY = os.getenv('TEST_ONLY')
test = testEnv.Command(
    f"build/test/{NAME}.log", testProg,
    f"build/test/{NAME} " + ("" if TEST_ONLY is None else f"--gtest_filter={TEST_ONLY}") + f" | tee build/{NAME}.log"
)

coverage = testEnv.Command(
    "build/test/coverage.info",
    [Glob("build/test/*.gcda"), Glob("build/main/*.gcda")],
    "lcov -c -d build/test -o build/test/coverage.info && genhtml build/test/coverage.info -o coverage"
)
testEnv.Depends(coverage, test)

testEnv.Alias("test", coverage)
testEnv.Clean(coverage, ["coverage", "build/test"])

docs = testEnv.Command("html/index.html", [], "doxygen Doxyfile")
testEnv.Clean(docs, "html")

testEnv.Alias("docs", docs)

# Assume you do not want repeating libraries' test/docs tasks everytime you invoke your project's test/docs task.
os.environ['SKIP'] = "test docs"

for dep in DEPENDENCIES:
    l = SConscript(f"{dep}/SConstruct")
    Depends(elf, l)
