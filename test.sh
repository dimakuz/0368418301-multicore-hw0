FIXTURE=$(mktemp)
NPROC=${2:-4}
for I in $(seq 100 100);
do
	echo -n "[$I]: "
	shuf -i 0-1844674407370955 -r -o $FIXTURE -n $(( $I * 1000 ))
	diff <($1 $NPROC $FIXTURE | sed '1d') <(sort -n $FIXTURE) && echo success|| echo failed
done
rm $FIXTURE
