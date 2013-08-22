#!/bin/sh

# Automatic testing script: compare program output from this compiler
# to that from C_COMPILER, which is considered valid.

# clang also works
C_COMPILER=gcc

# I like to use `md5sum' on Linux but some un*xes only
# have `sum', so yeah...
SUM_TOOL=sum

for x in test/*.c test/x86/*.c
do
	mkdir autotest-tmp
	SRC_FILE=autotest-tmp/test-temp.c
	echo "#include <stdio.h>" >$SRC_FILE
	/bin/echo "void echo(int n) { printf(\"%d\\n\", n); }" >>$SRC_FILE
	if ! grep -q main $x;
	then
		echo "int main(int argc, char **argv) {" >>$SRC_FILE
	fi
	# paste code, converting procedures syntax ("proc")
	# to standard C "int"
	cat $x | sed 's/proc/int/g' >>$SRC_FILE
	# gcc doesn't allow labels at the end of a codeblock,
	# but I do, so stick in a dummy statement for compatibility.
	# (see e.g. test/goto.c)
	if ! grep -q main $x;
	then
		echo 'argc = argc; ' >>$SRC_FILE
		echo "}" >>$SRC_FILE
	fi
	$C_COMPILER $SRC_FILE -o ./autotest-tmp/exec 2>/dev/null
	good_result=$(./autotest-tmp/exec | $SUM_TOOL)
	blok_result=$(./compile-run-x86.sh $x 2>/dev/null | $SUM_TOOL)
	if [ "$good_result" = "$blok_result" ];
	then
		echo "$x - PASS"
	else
		echo "$x - FAIL"
	fi
	rm -rf autotest-tmp/
done
