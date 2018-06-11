#!/bin/bash

mkdir -p layers
maxSize=0 #Calculate the maxsize of the output to format other frames
for i in $(seq -f "%03g" 1 $1)
do
    ./../../mcmap -from 112 -22 -to 112 -22 -height $i -north ~/.minecraft/saves/Super\ Street\ Profiler\ \(original\)\ -\ old
    convert output.png -alpha on -gravity South -background transparent -extent 74x202 layers/layer_$i.png
done

cd layers
convert -delay 10 -loop 0 -layers OptimizeFrame *.png chunk.gif
