#!/bin/bash

if [ $# -lt 3 ] ; then
    echo "Usage: bash createGif.sh -height <n> [other mcmap options]"
    exit 1
fi

mcmap="./../../mcmap"
options=""
height=0
tmpFolder="/tmp/mcmap_$$"

while true ; do
    case $1 in
	-height) height=$2; shift 2;;
	"") break;;
	*) options="$options $(echo $(printf '%q' "$1"))"; shift 1;;
    esac
done

mkdir -p $tmpFolder
rm -f $tmpFolder/*

echo "$mcmap $options" | bash
mv output.png $tmpFolder/layer_$height.png
maxSize=$(convert $tmpFolder/layer_$height.png -print "%wx%h" /dev/null)

for i in $(seq -f "%03g" 1 $[ $height - 1 ])
do
    echo "$mcmap $options -height $i" | bash
    convert output.png -alpha on -gravity South -background transparent -extent "$maxSize" "$tmpFolder/layer_$i.png"
done

convert -delay 10 -loop 0 -layers OptimizeFrame $tmpFolder/* chunk.gif

#rm -r $tmpFolder
