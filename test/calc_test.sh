#/usr/bin/env bash
PNAME='crypti'
path=`dirname $0`
N=3

for ((i=1; $i < $N + 1; i= $i + 1))
do
	err=`cat $path/test_$i.txt | $path/../$PNAME 2>/dev/null| diff - $path/answer_$i.txt`

	if [ -n "$err" ]
	then
	        echo "$path/test_$i.txt";
	        echo "$err";
	fi

done

