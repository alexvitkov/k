if [ a$1 == a-u ]; then
    KC=./compiler
else
    KC=./stable_compiler 
fi

echo Building with KC=$KC

${KC} *.k > k.asm || exit 1
nasm -felf64 k.asm -o k.o || exit 1

gcc -g *.c k.o -o compiler -no-pie
