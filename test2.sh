#!/bin/bash
FIXTURE=$(mktemp)
shuf -i 0-1844674407370955 -r -o $FIXTURE -n 5000000
for NPROC in $(seq 1 4);
do
	echo -n "[$NPROC]: "
	diff <($1 $NPROC $FIXTURE | sed '1d') <(sort -n $FIXTURE) && echo success|| echo failed
done
rm $FIXTURE
