#!/bin/sh

# wcc: wannabe c compiler command for unixlikes running amd64

if [ $(uname | grep MINGW) ];
then

gcc -DTARGET_AMD64 -O3 -o wcc64.exe -DMINGW_BUILD -DWCC codegen_amd64.c diagnostics.c general.c main.c optimize.c parser.c tokenizer.c tokens.c tree.c typeof.c escapes.c hashtable.c preprocess.c constfold.c global_initializer.c


else

c99 -g -DTARGET_AMD64 -O3 -o wcc64 -DWCC codegen_amd64.c diagnostics.c general.c main.c optimize.c parser.c tokenizer.c tokens.c tree.c typeof.c escapes.c hashtable.c preprocess.c constfold.c global_initializer.c

fi
