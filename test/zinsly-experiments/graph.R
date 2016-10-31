#!/usr/bin/env Rscript

library("ggplot2")

t = read.table("table")
ggplot(t, aes(V1, V7)) 
    + geom_bar(aes(fill = V2), position = "dodge", stat="identity") 
    + xlab("Benchmark") + ylab("Número de Regiões") + labs(fill="RFT")
