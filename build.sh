# build.sh
# @bingxio - https://drift-lang.fun/
CC=gcc

if [ -n "$1" ]; then
	if [ $1 == "-mod" ]; then
		for f in `ls ./module/*.c`; do
			$CC -fPIC -shared $f -o `basename $f .c`.so
		done
		echo "Done!"
		exit;
	fi
fi

OUT=""

for f in `ls ./src/*.c`; do
	$CC -std=c99 -c -g $f
	OUT="$OUT `basename $f .c`.o"
done

BUG=""

if [ -n "$1" ]; then
	if [ $1 == "-bug" ]; then
		BUG="-fsanitize=address"
	else
		echo "unknown arg: $1"
	fi
fi

$CC $BUG $OUT -Wl,-E -ldl -o drift

rm -f *.o
echo "Done!"
