#!/bin/bash

gcc $1 -Os -c -fno-asynchronous-unwind-tables -fno-stack-protector \
  -fomit-frame-pointer -fno-ident -T init.ld -o .a.o

objcopy -O binary --only-section=.text.* .a.o .a.text
objcopy -O binary --only-section=.data .a.o .a.data

trace_tool.bin .a.text .a.data

rm .a.*
