# Automatic testing script: compare program output from this compiler
# to that from C_COMPILER, which is considered valid.

# change to clang or whatever if you like ;)
C_COMPILER=gcc

for x in test/*
do
	mkdir autotest-tmp
	SRC_FILE=autotest-tmp/test-temp.c
	echo "#include <stdio.h>" >$SRC_FILE
	/bin/echo "void echo(int n) { printf(\"%d\\n\", n); }" >>$SRC_FILE
	echo "int main(int argc, char **argv) {" >>$SRC_FILE
	cat $x >>$SRC_FILE
	echo 'argc = argc; ' >>$SRC_FILE	# hurr durr
	echo "}" >>$SRC_FILE

	$C_COMPILER $SRC_FILE -o ./autotest-tmp/exec
	good_result=$(./autotest-tmp/exec | md5sum)
	blok_result=$(./compile-run.sh $x | grep -v 'BPF' | md5sum)
	if [ "$good_result" = "$blok_result" ];
	then
		echo "$x - PASS"
	else
		echo "$x - FAIL"
	fi
	rm -rf autotest-tmp/
done
