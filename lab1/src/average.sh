#!/bin/bash
sum=0
count="$#"
for num in "$@"; do
    sum=$((sum + num))
done
aver=$((sum / count))

echo "Среднее: $aver"
