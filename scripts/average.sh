#!/usr/bin/env bash

# Requires ImageMagick
# Outputs the color average of the image passed as an argument

set -eo pipefail

printf '%s\t%s\n' \
  "$(basename "${1}")" \
  "$(convert "${1}" -resize 1x1 txt:- | grep -Po "#[[:xdigit:]]{6}" | tr A-F a-f)"
