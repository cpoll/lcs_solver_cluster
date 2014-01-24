declare j=0;
for i in 03 04 05 06 07 08 09 {10..40}
do

	if [ $j -lt $1 ]
	then
		echo "Starting on cs"$i
		ssh cs$i -q ~/398/pollcris/a4/lcs.sh $2 $3 $4 $5 $6 &
	fi
	
	let j=j+1;
done
