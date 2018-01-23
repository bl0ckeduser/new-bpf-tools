#!/bin/sh

if [ $(uname | grep MINGW) ];
then

gcc -DTARGET_AMD64 -DMINGW_BUILD -g -std=c99 codegen_amd64.c diagnostics.c general.c main.c optimize.c parser.c tokenizer.c tokens.c tree.c typeof.c escapes.c hashtable.c preprocess.c constfold.c global_initializer.c -o compiler.exe

else

c99 -g -DTARGET_AMD64 codegen_amd64.c diagnostics.c general.c main.c optimize.c parser.c tokenizer.c tokens.c tree.c typeof.c escapes.c hashtable.c preprocess.c constfold.c global_initializer.c

fi
