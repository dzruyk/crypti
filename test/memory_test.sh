#/usr/bin/env bash
PNAME='crypti'
N=4

path=`dirname $0`

echo 'statesments tests...';
for ((i=1; $i<$N + 1; i=$i+1))
do
	cat $path/test_$i.txt | valgrind $path/../$PNAME 2>&1 | grep "in use at exit"
done

echo 'function tests...';

N=3
for ((i=1; $i<$N +1; i=$i+1))
do
	cat $path/test_func_$i.txt | valgrind $path/../$PNAME 2>&1 | grep "in use at exit"
done

echo 'cycle tests...'
N=1
for ((i=1; $i<$N +1; i=$i+1))
do
	cat $path/test_cycle_$i.txt | valgrind $path/../$PNAME 2>&1 | grep "in use at exit"
done

echo 'Done'
