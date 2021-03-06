#!/usr/bin/Rscript

library("ggplot2")
source("graphutils.R")

data <- loadTable("tableonlyuser4.csv")
data <- sortByBenchmark(data)
data <- normalize(data, "net50")
#data <- filterRFT(data, c("tt", "net100", "net50", "mret25", "mret50", "lei", "netplus"))
#data <- sortByRFT(data, c("lef"))
data <- filterRFT(data, c("tt", "net50"))
data <- sortByRFT(data, c("net100", "mret50", "mret25", "lei", "netplus", "lef"))

genAndSaveAllGraphs(data, "all", function(data, column) 
  genBarGraph(data, X = data$V1, Y = data[,column], FILL = data$V2, "Benchmarks", getColumnName(column), TRUE)
)

data$type = ifelse(isFromBench(data, c("IE", "finereader", "word", "powerpoint", "gedit", "gnumeric", "gnome-weather", "gimp")), "Desktop App", "Spec")

genAndSaveAllGraphs(data, "point", function(data, columnX, columnY) 
  genPointGraph(data, X = data[, columnX], Y = data[, columnY], FILL = data$V2, getColumnName(columnX), getColumnName(columnY))
, TRUE)

groupedspec    <- groupByBench(data, !isFromBench(data, 
            c("IE", "finereader", "word", "powerpoint", "gedit", "gnumeric", "gnome-weather", "gimp")), geo_mean)
groupedsysmark <- groupByBench(data, isFromBench(data, 
            c("IE", "finereader", "word", "powerpoint", "gedit", "gnumeric", "gnome-weather", "gimp")), geo_mean)

groupedspec$type    <- ".SPEC"
groupedsysmark$type <- "Desktop Apps"
grouped <- rbind(groupedspec, groupedsysmark)

genAndSaveAllGraphs(grouped, "group", function(data, column) 
  genBarGraph(data, X = data$type, Y = data[, column+1], FILL = data$Group.1, "Benchmarks", getColumnName(column), FALSE, FALSE, FALSE)
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
