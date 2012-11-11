#/usr/bin/env bash
PNAME='bin/crypti'
N=4

path=`dirname $0`
temp_file=`tempfile`
out='/dev/null'

echo 'statesments tests...';
for ((i=1; $i<$N + 1; i=$i+1))
do
	cat $path/test_$i.txt | valgrind $path/../../$PNAME 1>$out 2>$temp_file
	cat $temp_file | grep "in use at exit"
	cat $temp_file | grep "total heap usage:"
done

echo 'function tests...';

N=3
for ((i=1; $i<$N +1; i=$i+1))
do
	cat $path/test_func_$i.txt | valgrind $path/../../$PNAME 1>$out 2>$temp_file
	cat $temp_file | grep "in use at exit"
	cat $temp_file | grep "total heap usage:"
done

echo 'cycle tests...'
N=1
for ((i=1; $i<$N +1; i=$i+1))
do
	cat $path/test_cycle_$i.txt | valgrind $path/../../$PNAME 1>$out 2>$temp_file
	cat $temp_file | grep "in use at exit"
	cat $temp_file | grep "total heap usage:"
done

rm $temp_file
echo 'Done'
