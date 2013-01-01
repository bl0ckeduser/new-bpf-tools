if [ $(uname | grep MINGW) ];
then

# MinGW

./compiler.exe <$1 >compiler_temp.s && gcc compiler_temp.s -o test_prog.exe && ./test_prog.exe
rm -f compiler_temp.s test_prog.exe

else

# Unix / GNU / Linux 

# clang on FreeBSD has also been tested
COMPILER=gcc

./a.out <$1 >compiler_temp.s && $COMPILER -m32 compiler_temp.s -lc -o test_prog && ./test_prog
rm -f compiler_temp.s test_prog


fi
