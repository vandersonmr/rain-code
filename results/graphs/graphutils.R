#!/usr/bin/Rscript

library("ggplot2")
library("hashmap")

lines <- c(7,9,10,11,12,13,14,16,17,18,19,20,26,27)
names <- c("Total of Regions", "Avg. Dynamic Region Size",
           "Avg. Static Region Size", "Dynamic Region Coverage",
           "Region Code Duplication", "Completion Ratio",
           "Number of Compilations", "Number of Regions Transitions",
           "Number of Used Counters", "Spanned Cycle Ratio", 
           "Spanned Execution Ratio", "70% Cover Set",
           "Cold Regions (<1000)", "Cold Regions (Avg. < 200)")
h <- hashmap(lines, names)

getColumnName <- function(column) {
  return(h[[column]])
}

loadTable <- function(PATH) {
  data <- read.table(PATH, stringsAsFactors = FALSE)
  data$V14 = data$V14
  return(data)
}

normalize <- function(data, TECH) {
  for (i in 3:27) {
    data[,i] = mapply(function(a, c) { c / data[data$V1 == a & data$V2 == TECH, ][,i]  },  data$V1, data[,i]) 
  }
  return(data)
}

isFromBench <- function(data, NAMES) {
  bool = data[1] == ""
  for (name in NAMES) {
    bool = (data[1] == name | bool)
  }
  return(bool)
}

sortByBenchmark <- function(data) {
  sysmark_benchs = isFromBench(data, c("IE", "word", "finereader", "powerpoint","gimp","gedit","gnome-weather", "gnumeric")) 
  sorted = rbind(data[!sysmark_benchs,], data[sysmark_benchs,])
  sorted$V1 = factor(sorted$V1, levels = unique(sorted$V1))
  rownames(sorted) <- NULL
  return(sorted)
}

filterRFT <- function(data, RFTS) {
  bool = data$V2 != ""
  for (name in RFTS) {
    bool = (data$V2 != name & bool)
  }
  return(data[bool,])
}

sortByRFT <- function(data, RFTSORDER) {
  data$V2 <- factor(data$V2, levels = RFTSORDER)
  return(data)
}

genBarGraph <- function(data,X, Y, FILL, XLAB, YLAB, HLINE, VLINE = TRUE, ANNOT = TRUE) {
  p <- ggplot(data, aes(X, Y)) + geom_bar(aes(fill = FILL), position = "dodge", stat="identity") + 
    theme(text = element_text(size=20), axis.text.x = element_text(angle=30, hjust=1)) + xlab(XLAB) + ylab(YLAB)

  if (ANNOT)
    p <- p + annotate("text", x = 1.5, y = max(Y), label = "SPEC") + 
             annotate("text", x = 15.5, y = max(Y), label = "Desktop Apps")

  if (VLINE)
    p <- p + geom_vline(xintercept = 14.5, linetype = "longdash") + labs(fill="RFT") 

  if (HLINE)
    p <- p + geom_hline(yintercept = 1, colour="red") 

  return(p)
}

genPointGraph <- function(data,X, Y, FILL, XLAB, YLAB) {
  p <- ggplot(data, aes(X, Y)) + geom_point(aes(colour = FILL, shape = FILL), size=6) + 
    theme(text = element_text(size=20)) + xlab(XLAB) + ylab(YLAB)
  return(p)
}

genAndSaveAllGraphs <- function(data, PREFIX, GENFUNC, POINT=FALSE) {
  if (POINT) {
    set = c(9,10,12,13,14,16,17,19,27);
    for (i in set) {
      for (j in set) {
        if (j > i) {
          p <- GENFUNC(data, i, j)
          ggsave(paste(PREFIX,i,"x",j,".png", sep=""), p, width = 10, height = 10)
        }
      }
    }
  } else {
    for (i in lines) {
      p <- GENFUNC(data, i)
      ggsave(paste(PREFIX, i, ".png", sep=""), p, width = 16, height = 7)
    }
  }
}

groupByBench <- function(data, BOOL, FUNC) {
  return(aggregate(data[BOOL,], list(data[BOOL,]$V2), FUNC))
}

geo_mean <- function(data) {
  if (is.factor(data) | !is.numeric(data))
    return(NA);

  log_data <- log(data)
  gm <- exp(mean(log_data[is.finite(log_data)]))
  return(gm)
}

cohens_d <- function(x, y) {
    lx <- length(x)- 1
    ly <- length(y)- 1
    md  <- abs(mean(x) - mean(y))        
    csd <- lx * var(x) + ly * var(y)
    csd <- csd/(lx + ly)
    csd <- sqrt(csd)
    cd  <- md/csd    
}
