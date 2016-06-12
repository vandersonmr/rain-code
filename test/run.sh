#!/bin/bash

rm -r results;
mkdir results; cd results
for source in $(cd ..; ls *.c); do
  mkdir $source; cd $source;
  ../../../tracegenerator/generate.sh ../../$source
  mv test.bin.gz test.0.bin.gz
  rain_tool.bin -b test -s 0 -e 0 -lt
  dot -Tpdf *.dot -o test.pdf
  mkdir entrada; mv test.0.bin.gz entrada/
  mkdir results
  ../../net test 0 0
  cd ..;
done;
cd ..;
