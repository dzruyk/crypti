#/usr/bin/env bash
PNAME='crypti'
N=4

path=`dirname $0`

for ((i=1; $i<$N + 1; i=$i+1))
do
	cat $path/test_$i.txt | valgrind $path/../$PNAME 2>&1 | grep "in use at exit"
done
