mkdir -p build/
gcc -c -std=c99 src/main.c -o build/main.o
gcc -c -std=c99 src/emu.c -o build/emu.o
gcc -std=c99 build/main.o build/emu.o -o build/lbmaid
