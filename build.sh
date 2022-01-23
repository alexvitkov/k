KC=./stable_compiler 

${KC} Lex.k > Lex.k.asm
nasm -felf64 Lex.k.asm -o Lex.k.o

gcc -g *.c *.o -o compiler
