declare j=0;
for i in 02 03 04 05 06 07 08 09 {10..40}
do

	if [ $j -lt $1 ]
	then
		echo "Cleaning up cs"$i		
                ssh cs$i -qt "killall lcs.sh; killall lcs.out"
	fi
	
	let j=j+1;
done
