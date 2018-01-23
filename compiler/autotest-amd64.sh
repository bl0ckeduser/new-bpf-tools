#!/bin/sh

# Automatic testing script: compare program output from this compiler
# to that from C_COMPILER, which is considered valid.

# clang also works
C_COMPILER=cc

# I like to use `md5sum' on Linux but some un*xes only
# have `sum', so yeah...
SUM_TOOL=sum

pass=0
passes=""
fail=0
fails=""

# multifile
for mf_dir in `ls test/amd64/multifile/`
do
	rm -rf autotest-tmp
	mkdir autotest-tmp
	x="test/amd64/multifile/$mf_dir/*.c"
	$C_COMPILER test/amd64/multifile/$mf_dir/*.c -o ./autotest-tmp/exec 2>/dev/null
	good_result=$(./autotest-tmp/exec | $SUM_TOOL)
	blok_result=$(./compile-run-amd64.sh test/amd64/multifile/$mf_dir/*.c 2>/dev/null | $SUM_TOOL)
	if [ "$good_result" = "$blok_result" ];
	then
		echo "$x - PASS"
		pass=$(expr $pass + 1)
		passes="$passes $x"
	else
		echo "$x - FAIL"
		fail=$(expr $fail + 1)
		fails="$fails $x"
	fi
	rm -rf autotest-tmp/
done

for x in test/amd64/std/*.c test/amd64/*.c
do
	rm -rf autotest-tmp
	mkdir autotest-tmp
	SRC_FILE=$x
	$C_COMPILER $SRC_FILE -o ./autotest-tmp/exec 2>/dev/null
	good_result=$(./autotest-tmp/exec | $SUM_TOOL)
	blok_result=$(./compile-run-amd64.sh $x 2>/dev/null | $SUM_TOOL)
	if [ "$good_result" = "$blok_result" ];
	then
		echo "$x - PASS"
		pass=$(expr $pass + 1)
		passes="$passes $x"
	else
		echo "$x - FAIL"
		fail=$(expr $fail + 1)
		fails="$fails $x"
	fi
	rm -rf autotest-tmp/
done

echo "----"
echo "totals:"
echo "passes: $pass ($passes)"
echo "fails: $fail ($fails)"
