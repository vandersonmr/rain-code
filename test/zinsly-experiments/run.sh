#!/bin/bash

# $1 must be the chosen technique (net, lei)
runRFT() {
	echo $1
	rm -fr results$1;
	mkdir -p results$1; 
	cd results$1;
	echo "Running $i";
	pwd
 	rain_tool.bin -b ../entrada/out -t $1 -s 201 -e 300 -lt	
	cd ..
}

cd spec;
for i in $(ls); do
	cd $i;
	runRFT $1 &
	cd ..;
done;
cd ..;
