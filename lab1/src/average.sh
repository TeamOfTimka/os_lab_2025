#!/bin/sh

echo "Number of arguments: $#"

sum1=0
for n in $@
do
  let sum1=$sum1+$n
done
echo "Average of args: $(($sum1 / $#))"