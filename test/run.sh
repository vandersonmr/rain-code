#!/bin/bash

rm -r results;
mkdir results; cd results
for source in $(cd ..; ls *.c); do
  mkdir $source; cd $source;
  ../../../tracegenerator/generate.sh ../../$source

  mv test.bin.gz test.0.bin.gz
  rain_tool.bin -t lei -b test -s 0 -e 0 -lt

  for file in $(ls *.dot); do
    dot -Tpdf $file -o $file.pdf
  done;

  mkdir entrada; 
  mv test.0.bin.gz entrada/
  mv .a.text entrada/a.text
  mv .a.data entrada/a.data

  mkdir results
  ../../net test 0 0
  mv results resultsNET

  mkdir results
  ../../lei test 0 0
  mv results resultsLEI

  cd ..;
done;
cd ..;
