# RAIn

**Version**: 1.0.0 (02-21-2017)

**Usage**: ./rain_tool.bin -b trace_path -s index -e index [-h] [-o stats.csv] [-mix] {-lt|-wt} [-bin path/binary] 

## DESCRIPTION:

This program implements the **RAIn (Region Appraisal Infrastructure)** and can be
used to investigate region formation strategies for dynamic binary
translators. For more information, please, read: Zinsly, R. "Region formation
techniques for the design of efficient virtual machines". MsC
Thesis. Institute of Computing, 2013 (in portuguese).
The tool takes as input a trace of instructions, emulate the formation and 
execution of  regions, and generates statistics about the region formation
techniques. 
The input trace may be store in multiple files, each one
containing a sub-sequence of the trace. Each file is named
BASENAME.INDEX.bin.gz, where trace_path is the trace_path of
the trace and INDEX indicates the sequence of the trace.
The user must provide the trace_path (-b), the start index (-s) and the end 
index (-e).

### ARGUMENTS:
  -b : input file trace_path
  -bin : input binary file path
  -d : depth limit for NETPlus
  -e : end: last file index
  -h : display the help message
  -lt : linux trace. System/user address threshold = 0xB2D05E00
  -mix : Allow user and system code in the same NET regions.
  -overall_stats : file name to dump overall statistics in CSV format
  -reg_stats : file name to dump regions statistics in CSV format
  -s : start: first file index 
  -t : RF Technique
  -wt : windows trace. System/user a *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version. ddress threshold = 0xF9CCD8A1C5080000

## Contributors

 - @eborin Edson Borin (edson@ic.unicamp.br)
 - @rzinsly Raphael Zinsly (raphael.zinsly@gmail.com)
 - @vandersonmr Vanderson M. Rosario (vandersonmr2@gmail.com)

# License

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version. 
