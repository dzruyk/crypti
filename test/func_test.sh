#!/usr/bin/env bash
path=`dirname $0`

cat $path/test_func.txt | $path/../crypti
