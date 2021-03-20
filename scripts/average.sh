# Requires ImageMagick
# Outputs the color average of the image passed as an argument

echo -ne "`basename "$1"`\t"
convert "$1" -resize 1x1 txt:- | grep -Po "#[[:xdigit:]]{6}" | tr A-F a-f
