if ! test -e wcc
then
	echo "please compile the standalone wcc command via ./build-wcc.sh"
	exit 1
fi

if ! test -e wcc2
then
	echo "please compile phase 2 (wcc2) via selfcompile"
fi

./wcc2 -w -DWCC -g -o wcc3 tokenizer.c general.c tokens.c hashtable.c diagnostics.c main.c optimize.c tree.c preprocess.c escapes.c typeof.c codegen_x86.c parser.c constfold.c global_initializer.c

