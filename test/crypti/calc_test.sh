#/usr/bin/env bash
PNAME='bin/crypti'
path=`dirname $0`
N=9

echo "statesments tests ($N total) ...";
for ((i=1; $i < $N + 1; i= $i + 1))
do
	err=`cat $path/test_$i.txt | $path/../../$PNAME 2>/dev/null| diff - $path/answer_$i.txt`

	if [ -n "$err" ]
		then
		echo "$path/test_$i.txt";
		echo "$err";
	fi
done

N=1
echo "sorting tests ($N total)..."
for ((i=1; $i < $N + 1; i= $i + 1))
do
	err=`cat $path/test_sort_$i.txt | $path/../../$PNAME 2>/dev/null| diff - $path/answer_test_sort_$i.txt`

	if [ -n "$err" ]
		then
		echo "$path/test_sort_$i.txt";
		echo "$err";
	fi
done

echo 'Done'

