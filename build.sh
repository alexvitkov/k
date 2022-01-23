KC=./stable_compiler 

${KC} *.k > k.asm || exit 1
nasm -felf64 k.asm -o k.o || exit 1

gcc -g *.c k.o -o compiler
