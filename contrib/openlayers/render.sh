#!/bin/bash

#first argument should be the world
WORLD=$1
# where can we find mcmap
MCMAPBIN="mcmap"
# where shoud we put the processed tiles
TILESDIR="`pwd`/tiles"
# where do we put the raw tiles
RAWTILESDIR="`pwd`/tmp"

function usage(){
	echo "Usage:"
	echo $0 "<path to world>"
}

if [[ ! -d $WORLD ]] ; then
  usage
  exit 1
fi 

#make some folders to store stuff
if [[ ! -d $TILESDIR ]] ; then
	mkdir -p $TILESDIR
fi
if [[ ! -d $RAWTILESDIR ]] ; then
	mkdir -p $RAWTILESDIR
fi


#generate the raw tiles
$MCMAPBIN -split $RAWTILESDIR $WORLD

#process all raw tiles
echo "Processing raw tiles"
for SRC in $RAWTILESDIR/*.png ; do
	FILE="`basename $SRC`"
	#default tile size for OpenLayers is 256x256 pixels
	convert -scale 256 $SRC $TILESDIR/$FILE 
done
echo "Done"

