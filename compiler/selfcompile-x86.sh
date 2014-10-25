if ! test -e wcc
then
	echo "please compile the standalone wcc command via ./build-wcc.sh"
	exit 1
fi

./wcc -DWCC -g -o wcc2 tokenizer.c general.c tokens.c hashtable.c diagnostics.c main.c optimize.c tree.c preprocess.c escapes.c typeof.c codegen_x86.c parser.c constfold.c global_initializer.c

