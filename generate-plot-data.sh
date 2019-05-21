#!/bin/bash
timeplot="bck-`date '+%s'`"

awk '{printf("%s\n",$1)}' trace_100_0.5.dat > ycol.bfs
awk '{printf("%s\n",$2)}' trace_100_0.5.dat > xcol1.bfs
awk '{printf("%s\n",$2)}' trace_100_1.0.dat > xcol2.bfs
awk '{printf("%s\n",$2)}' trace_100_5.0.dat > xcol3.bfs
awk '{printf("%s\n",$2)}' trace_100_10.0.dat > xcol4.bfs

paste ycol.bfs xcol1.bfs xcol2.bfs xcol3.bfs xcol4.bfs > gplot-data-$timeplot.dat

#LDIR="bck-`date '+%s'`"
#mkdir datebck/$LDIR
#mv gplot_* datebck/$LDIR

rm *.bfs
