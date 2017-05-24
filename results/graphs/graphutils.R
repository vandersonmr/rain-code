#!/usr/bin/Rscript

library("ggplot2")
library("hashmap")

lines <- c(7,9,10,11,12,13,14,16,17,18,19,21,26,27,28)
names <- c("Total of Regions", "Avg. Dynamic Region Size",
           "Avg. Static Region Size", "Dynamic Region Coverage",
           "Region Code Duplication", "Completion Ratio",
           "Number of Compilations", "Number of Regions Transitions",
           "Number of Used Counters", "Spanned Cycle Ratio", 
           "Spanned Execution Ratio", "70% Cover Set", "Expasion Exec. Freq",
           "Cold Regions (<1000)", "Cold Regions (Avg. < 200)")
h <- hashmap(lines, names)

getColumnName <- function(column) {
  return(h[[column]])
}

loadTable <- function(PATH) {
  data <- read.table(PATH, stringsAsFactors = FALSE)
  data$V14 <- data$V14 + data$V7;
  data[data$V2 == "lei",]$V10 <- data[data$V2 == "lei",]$V10;
  data[data$V2 == "net50",]$V26 <- data[data$V2 == "net50",]$V3;
  return(data)
}

normalize <- function(data, TECH) {
  for (i in 3:28) 
    data[,i] = mapply(function(a, c) { c / data[data$V1 == a & data$V2 == TECH, ][,i]  },  data$V1, data[,i]) 
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
    theme(text = element_text(size=25), axis.text.x = element_text(angle=30, hjust=1)) + xlab(XLAB) + ylab(YLAB)

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
  p <- ggplot(data, aes(X, Y)) + geom_point(aes(colour = data$type, shape = FILL), size=6) + 
    theme(text = element_text(size=25)) + xlab(XLAB) + ylab(YLAB)
  return(p)
}

genAndSaveAllGraphs <- function(data, PREFIX, GENFUNC, POINT=FALSE) {
  if (POINT) {
    for (i in lines) {
      for (j in lines) {
        if (j > i) {
          if (!is.na(data[1,i]) & !is.na(data[1,j])) {
            #if (abs(cor(data[,i], data[,j])) > 0.1) {
              p <- GENFUNC(data, i, j)
              ggsave(paste(PREFIX,i,"x",j,".pdf", sep=""), p, width = 14, height = 9)
            #}
          }
        }
      }
    }
  } else {
    for (i in lines) {
      p <- GENFUNC(data, i)
      ggsave(paste(PREFIX, i, ".pdf", sep=""), p, width = 16, height = 8)
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
