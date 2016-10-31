#!/bin/bash

cd spec
for i in $(ls); do
	for j in net mret2 tt lef; do
		cd $i/results$j;
		echo -n "$i $j "
		cat overall_stats.csv | cut -d',' -f2 | awk '{printf "%s ", $1} END {printf "\n"}'; 
		cd ../..;
	done;
done;
cd ..


