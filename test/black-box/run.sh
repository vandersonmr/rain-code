#!/bin/bash

to_test="net mret2 tt lef"

compile() {
  gcc -O0 -fno-asynchronous-unwind-tables -fno-stack-protector \
    -fomit-frame-pointer -fno-ident -nostdlib -std=c99 *.c -o .a.o -c;
}

execute() {
  trace_tool.bin .a.o &&
  mv test.bin.gz test.0.bin.gz &&
  mkdir results
  for technique in $to_test; do
    rain_tool.bin -t $technique -b test -s 0 -e 0 -lt -bin .a.o
    mkdir results/$technique
    mv *.csv *.dot results/$technique
  done;
}

check() {
  for technique in $to_test; do
    words=$(echo "results/$technique" | wc -c)
    for file in $(ls results/$technique/*); do
      if ! diff $file expected/$technique/${file:$words}; then
        echo ${file:$words}
        return 1
      fi
    done
  done
}

clean() {
  rm -rf results
  rm -f .a.o test.0.bin.gz 
}

run_all_tests() {
  list_of_tests=$(ls -d */)

  oks=0
  total=0
  for test in $list_of_tests; do
    let total=$total+1
    echo -e "\e[1m>> Running test: $test\e[0m"
    cd $test
    if [[ "$test" != "real/" ]]; then
      clean
      if ! compile; then
        echo -e "\e[31mFailed during compilation\e[0m"
        cd ..
        continue
      fi
      if ! execute; then
        echo -e "\e[31mFailed during execution\e[0m"
        cd ..
        continue
      fi
      if ! check; then
        echo -e "\e[31mFailed during assert\e[0m"
        cd ..
        continue
      fi
    else 
      rm -rf results
      mv test.0.bin.gz test.bin.gz
      if ! execute; then
        echo -e "\e[31mFailed during execution\e[0m"
        cd ..
        continue
      fi
      if ! check; then
        echo -e "\e[31mFailed during assert\e[0m"
        cd ..
        continue
      fi
    fi; # real if
    cd ..
    echo -e ">> \e[34m[OK] $test\e[0m"
    let oks=$oks+1
  done;
  echo -e "\e[1m>>> Passed $oks/$total tests\e[0m"
}

echo ">>> Running Black Boxes tests"
run_all_tests;
echo ">>> Done"
