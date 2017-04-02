#!/usr/bin/Rscript

library("ggplot2")
source("graphutils.R")


data <- loadTable("tableonlyuser.csv")
nData <- normalize(data, "net50")
fData <- filterRFT(nData, c("net50", "lei35", "tt"))
fData <- sortByBenchmark(fData)
sData <- sortByRFT(fData, c("net100", "mret50", "mret25", "lei35", "netplus", "lef50", "tt"))

genAndSaveAllGraphs(sData, "all", function(data, column) 
  genGraph(data, X = data$V1, Y = data[,column], FILL = data$V2, "Benchmarks", getColumnName(column), TRUE)
)

groupedsysmark <- groupByBench(sData, isFromBench(sData, c("IE", "finereader", "word", "powerpoint")), geo_mean)
groupedspec    <- groupByBench(sData, !isFromBench(sData, c("IE", "finereader", "word", "powerpoint")), geo_mean)

groupedspec$type    <- "spec"
groupedsysmark$type <- "sysmark"
grouped <- rbind(groupedspec, groupedsysmark)

genAndSaveAllGraphs(grouped, "group", function(data, column, name) 
  genGraph(data, X = data$type, Y = data[, column+1], FILL = data$Group.1, "Benchmarks", getColumnName(column), TRUE, FALSE, FALSE)
)

##d <- read.table("hist", stringsAsFactors = FALSE)
#
##plot(density(d$V1))
#
#library(scales)
#
#t <- read.table("tablenetplus.csv")
#t$V2 <- c(10, 8, 6, 4, 2, 1)
#tfilter <- sorted[sorted$V1 == "gcc",]
#tfilter <- tfilter[tfilter$V2 == "net",]
#t$V9 <- (1-(tfilter$V9)/t$V9)
#pm <- ggplot(t, aes(V2, V9)) + geom_point(shape=5, size=3) + geom_line() +
#  scale_x_discrete(limit=c("1","2", " ", "4", " ", "6", " ", "8", " ", "10")) +
#  theme_bw() + xlab("Depth of Loop Search") +  
#  ylab("Average static region size") +
#  theme(text = element_text(size=18), axis.text.x = element_text(angle=0, hjust=1)) +
#  scale_y_continuous(labels=percent)
