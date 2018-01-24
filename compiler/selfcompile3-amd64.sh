if ! test -e wcc64
then
	echo "please compile the standalone wcc command via ./build-wcc-amd64.sh"
	exit 1
fi

if ! test -e wcc64_3
then
        echo "please compile phase 3 (wcc2) via selfcompile"
fi


./wcc64_3 -DTARGET_AMD64 -DWCC -o wcc64_4 tokenizer.c general.c tokens.c hashtable.c diagnostics.c main.c optimize.c tree.c preprocess.c escapes.c typeof.c codegen_amd64.c parser.c constfold.c global_initializer.c
./wcc64_3 -DTARGET_AMD64 -o wcc64_4b tokenizer.c general.c tokens.c hashtable.c diagnostics.c main.c optimize.c tree.c preprocess.c escapes.c typeof.c codegen_amd64.c parser.c constfold.c global_initializer.c
