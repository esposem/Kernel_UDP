# call 'plot_bars.p' "kernel_result.txt"
# gnuplot --persist -c plot_bars.p $filename

filename=ARG1



set term qt title "Bars" (0)
reset
set xlabel "Number Clients"
set grid
set yrange [0<*:]
#set xrange [0 : 60]
set boxwidth -2

set multiplot
set ylabel "Troughput (mess/sec)"
set size 1,0.5
set origin 0,0.5
#set yrange [0 : 600000]
plot filename i 0 u 1:2:2:4:6:xtic(1) w boxerrorbars lc rgb"black" t "50 percentile"
clear
#set format y '%.0s%c'
a= GPVAL_X_MIN-5
b=GPVAL_X_MAX*1.1
c= GPVAL_Y_MIN
d=GPVAL_Y_MAX*1.1
set xrange [a:b]
set yrange [c:d]
plot filename i 0 u 1:2:2:4:6:xtic(1) w boxerrorbars lc rgb"black" t "50 percentile", \
     #filename i 0 u 1:2:xtic(1) w lines lc rgb"black" dt 2 lw 0.7 t "50 percentile" smooth unique, \
     #filename i 0 u 1:4:xtic(1) w lines lc rgb"red" dt 3 lw 0.7 t "99 percentile" smooth unique

set ylabel "Latency (us)"
set size 1,0.5
set origin 0,0
#set format y '%.0s%c'
#set yrange [0 : 120]
unset yrange
plot filename i 0 u 1:3:3:5:6:xtic(1) w boxerrorbars lc rgb"black" t "50 percentile", \
     #filename i 0 u 1:5:xtic(1) w lines lc rgb"red" dt 3 lw 0.7 t "99 percentile" smooth unique, \
     #filename i 0 u 1:3:xtic(1) w lines lc rgb"black" dt 2 lw 0.7 t "50 percentile" smooth unique
unset multiplot



set term qt title "Troughput vs Latency" (1)
reset
set ylabel "Latency (us)"
set xlabel "Troughput (mess/sec)"
set format y '%.0s%c'
set format x '%.0s%c'
set grid
set yrange [0<*:]
plot filename i 0 u 2:3 w lines lc rgb"black" notitle smooth unique
#plot for [IDX=0:*] filename i 0 i IDX u 1:2:3 w boxes title columnheader(1) #lw 1



set term qt title "Troughput and Latency" (2)
reset
set xlabel "Number Clients"
set grid
set yrange [0<*:]
#set xrange [0 : 60]

set multiplot
set ylabel "Troughput (mess/sec)"
set size 1,0.5
set origin 0,0.5
#set format y '%.0s%c'
#set yrange [0 : 600000]
plot filename i 0 u 1:2:xtic(1) w lines lc rgb"black" dt 1 lw 0.7 t "50 percentile" smooth unique, \
filename i 0 u 1:4:xtic(1) w lines lc rgb"red" dt 1 lw 0.7 t "99 percentile" smooth unique

set ylabel "Latency (us)"
set size 1,0.5
set origin 0,0
#set format y '%.0s%c'
#set yrange [0 : 120]
plot filename i 0 u 1:5:xtic(1) w lines lc rgb"red" dt 1 lw 0.7 t "99 percentile" smooth unique, \
filename i 0 u 1:3:xtic(1) w lines lc rgb"black" dt 1 lw 0.7 t "50 percentile" smooth unique
unset multiplot

set term qt title "CDF" (3)
clear
reset
set ylabel "Percentage"
set xlabel "Latency (us)"
set format y '%.0s\%%'
set format x '%.0s%c'
set grid
set yrange [0:100]
#set xrange [-50<*:]
plot filename i 1 u 1:2 w linespoints lc rgb"red" title columnheader(0) smooth unique
#plot for [IDX=0:*] filename i IDX u 1:2:3 w boxes title columnheader(1) #lw 1
