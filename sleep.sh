#!/bin/sh

for i in `seq 1 $1` ; do
  sleep 1
  echo "sleep.sh: $i"
done
