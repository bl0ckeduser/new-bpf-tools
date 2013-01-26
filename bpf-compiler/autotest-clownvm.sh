# Automatic testing script: compare program output from this compiler
# to that from C_COMPILER, which is considered valid.

# change to clang or whatever if you like ;)
C_COMPILER=gcc

# the examples not ending in .c are not C-compatible
for x in test/*.c
do
	mkdir autotest-tmp
	SRC_FILE=autotest-tmp/test-temp.c
	echo "#include <stdio.h>" >$SRC_FILE
	/bin/echo "void echo(int n) { printf(\"%d\\n\", n); }" >>$SRC_FILE
	echo "int main(int argc, char **argv) {" >>$SRC_FILE
	cat $x >>$SRC_FILE
	# gcc doesn't allow labels at the end of a codeblock,
	# but I do, so stick in a dummy statement for compatibility.
	# (see e.g. test/goto.c)
	echo 'argc = argc; ' >>$SRC_FILE
	echo "}" >>$SRC_FILE

	$C_COMPILER $SRC_FILE -o ./autotest-tmp/exec
	good_result=$(./autotest-tmp/exec | md5sum)
	blok_result=$(./a.out < $x | md5sum)
	if [ "$good_result" = "$blok_result" ];
	then
		echo "$x - PASS"
	else
		echo "$x - FAIL"
	fi
	rm -rf autotest-tmp/
done
