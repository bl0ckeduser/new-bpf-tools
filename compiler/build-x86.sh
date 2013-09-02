if [ $(uname | grep MINGW) ];
then

gcc -DMINGW_BUILD -g -std=c99 codegen_x86.c diagnostics.c general.c main.c optimize.c parser.c tokenizer.c tokens.c tree.c typeof.c escapes.c -o compiler.exe

else

c99 codegen_x86.c diagnostics.c general.c main.c optimize.c parser.c tokenizer.c tokens.c tree.c typeof.c escapes.c

fi
