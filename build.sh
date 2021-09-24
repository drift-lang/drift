echo "1(3): build target file.."
gcc -std=c99 -c -g src/compiler.c
gcc -std=c99 -c -g src/keg.c
gcc -std=c99 -c -g src/lexer.c
gcc -std=c99 -c -g src/main.c
gcc -std=c99 -c -g src/object.c
gcc -std=c99 -c -g src/table.c
gcc -std=c99 -c -g src/type.c
gcc -std=c99 -c -g src/vm.c

echo "2(3): build execution file.."
OUT="compiler.o keg.o lexer.o main.o object.o table.o type.o vm.o"
#gcc -fsanitize=address $OUT -Wl,-E -ldl -o drift
gcc $OUT -Wl,-E -ldl -o drift

echo "3(3): build standard library.."
gcc -fPIC -shared module/list.c -o list.so
gcc -fPIC -shared module/os.c -o os.so
gcc -fPIC -shared module/str.c -o str.so
gcc -fPIC -shared module/strconv.c -o strconv.so
gcc -fPIC -shared module/sys.c -o sys.so

rm -f *.o
echo "Done!"
