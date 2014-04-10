MY_COMPILER="./a.out"
TEST_PROG="test_prog"

if [ $(uname | grep MINGW) ];
then
	TEST_PROG="testprog.exe"
	MY_COMPILER="./compiler.exe"
fi

# clang on FreeBSD has also been tested
COMPILER=gcc

asmfiles=""
for file in $@
do
	asmfile="$(basename $(echo $file)).s"
	if ! $MY_COMPILER <$file >$asmfile
	then
		echo "Failed to compile file: $file"
		exit 1
	fi
	asmfiles="$asmfiles$asmfile "
done

if ! $COMPILER -m32 $asmfiles -lc -o $TEST_PROG
then
	exit 1
fi

./$TEST_PROG

rm -f $asmfiles $TEST_PROG


