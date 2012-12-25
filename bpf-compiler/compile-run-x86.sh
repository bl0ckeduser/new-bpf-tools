# clang on FreeBSD has also been tested
COMPILER=gcc

./a.out <$1 >compiler_temp.s && $COMPILER -m32 compiler_temp.s -lc -g -o test_prog && ./test_prog
rm -f compiler_temp.s test_prog



