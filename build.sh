KC=./stable_compiler 

${KC} Lex.k > Lex.k.asm || exit 1
nasm -felf64 Lex.k.asm -o Lex.k.o || exit 1

gcc -g *.c *.o -o compiler
