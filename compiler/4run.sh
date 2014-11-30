MY_COMPILER="./wcc4b"
TEST_PROG="test_prog"

if [ $(uname | grep MINGW) ];
then
	TEST_PROG="testprog.exe"
	MY_COMPILER="./compiler.exe"
	LC_FLAG=""
else
	LC_FLAG="-lc"
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
		rm $asmfiles
		exit 1
	fi
	asmfiles="$asmfiles$asmfile "
done

if ! $COMPILER -m32 $asmfiles $LC_FLAG -o $TEST_PROG
then
	rm -f $asmfiles $TEST_PROG
	exit 1
fi

./$TEST_PROG

rm -f $asmfiles $TEST_PROG

