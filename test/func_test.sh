#!/usr/bin/env bash
path=`dirname $0`
PNAME='crypti'
N=3

for ((i=1; $i < $N + 1; i= $i + 1))
do
	cat $path/test_func_1.txt | $path/../crypti
done

