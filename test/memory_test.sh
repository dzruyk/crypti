#/usr/bin/env bash
PNAME='crypti'
N=4


for ((i=1; $i<$N + 1; i=$i+1))
do
	cat ./test_$i.txt | valgrind ../$PNAME 2>&1 | grep "in use at exit"
done
