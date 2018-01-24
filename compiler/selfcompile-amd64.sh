if ! test -e wcc64
then
	echo "please compile the standalone wcc command via ./build-wcc.sh"
	exit 1
fi

./wcc64 -DWCC -DTARGET_AMD64 -o wcc64_2 tokenizer.c general.c tokens.c hashtable.c diagnostics.c main.c optimize.c tree.c preprocess.c escapes.c typeof.c codegen_amd64.c parser.c constfold.c global_initializer.c

