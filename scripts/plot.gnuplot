# run with:
# gnuplot -e "filename='<path to profile>'" plot.gnuplot

set datafile separator '\t'

set xlabel 'Time (s)'
set xtics rotate

set ylabel 'Memory (kB)'

set grid

set key autotitle columnhead
set key right bottom

set terminal qt persist

plot filename using 1:2 with lines, '' using 1:3 with lines, '' using 1:4 with lines, '' using 1:5 with lines
