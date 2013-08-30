#!/bin/sh

# 	This script  must be run from its directory.
#	Warning: waterloo's UNIX_OR_MAC.zip file is 13 MB in size.
#	Uncomment the below two lines if you don't have the .zip file yet
#	and/or have not unpacked it yet

#wget http://cemc.uwaterloo.ca/contests/computing/2012/stage1/Stage1Data/UNIX_OR_MAC.zip
#unzip UNIX_OR_MAC.zip

cp ../../../compile-run-x86.sh .
cp ../../../a.out .

ptot=0
ftot=0
for prob in s5
do
	pass=0
	fail=0
	for cas in `ls mac_nix_data/senior/S5/*.in`
	do
		chkfile=`echo $cas | sed 's/in/out/'`
		out=`cat $cas | (timeout -k 1 1 ./compile-run-x86.sh ccc2012-s5.c 2>/dev/null) | sum`
		chk=`cat $chkfile | sum`
		if [ "$chk" = "$out" ]
		then
			echo ${cas}: PASS
		else
			echo ${cas}: FAIL
		fi
	done
done
