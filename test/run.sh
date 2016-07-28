#!/bin/bash

rm -r results;
mkdir results; cd results
for source in $(cd ..; ls *.c); do
  echo $source
  mkdir $source; cd $source;
  ../../../tracegenerator/generate.sh ../../$source

  mv test.bin.gz test.0.bin.gz
  
  rain_tool.bin -t tt -b test -s 0 -e 0 -lt

  for file in $(ls *.dot); do
    dot -Tpdf $file -o $file.pdf
  done;

  mkdir entrada;
  mv test.0.bin.gz entrada/
  mv .a.text entrada/a.text
  mv .a.data entrada/a.data

  for rft in net lei mret2 tt; do
    mkdir results
    ../../$rft test 0 0
    mv results results$rft
  done;

  cd ..;
done;
cd ..;
