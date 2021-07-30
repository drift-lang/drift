for f in $(ls ./tests/*.ft)
do
	# if [ $f == 'test.ft' ]; then
	#	continue
	# fi
	# read -n 1 -p "Type to next.." > _
	echo -e "=== RUNNING: \"$f\" ==="
	cat $f
	time drift -b $f
done
