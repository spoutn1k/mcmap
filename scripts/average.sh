#!/usr/bin/env bash

# Requires ImageMagick
# Outputs the color average of the image passed as an argument

set -eo pipefail

SYSTEM=$(uname)
EXTRACTDIR=/tmp/extracted_blocks

macos::install_deps() {
  # Script requires imagemagick
  if ! command -v convert &>/dev/null; then
    if [[ "$SYSTEM" == "Darwin" ]] && command -v brew &>/dev/null; then
      brew install imagemagick
    fi
  fi
}

macos::mc_home() {
  echo "$HOME/Library/Application Support/minecraft/versions"
}

linux::mc_home() {
  echo "$HOME/.minecraft/versions"
}

unpack_assets() {
  case "$SYSTEM" in
      Darwin)
          MC_HOME="$(macos::mc_home)"
          ;;
      Linux)
          MC_HOME="$(linux::mc_home)"
          ;;
  esac

  if [[ -z "$MINECRAFT_VER" ]]; then
    return
  fi

  JAR="$MC_HOME/$MINECRAFT_VER/$MINECRAFT_VER.jar"

  if [[ -n "$JAR" ]]; then
    mkdir -p "$EXTRACTDIR/$MINECRAFT_VER"
    pushd "$EXTRACTDIR/$MINECRAFT_VER"
    jar xf "$JAR" assets/minecraft/textures/block
    popd
  fi
}

average() {
  FILE=$1
  EXTRACTED="$EXTRACTDIR/$MINECRAFT_VER/assets/minecraft/textures/block/$1.png"
  if [[ -f "$EXTRACTED" ]] ; then
      FILE="$EXTRACTED"
  fi

  COLOR="$(convert "$FILE" -resize 1x1 txt:-     \
      | grep -o "#[[:xdigit:]]\{6\}"            \
      | tr A-F a-f)"

  printf '%s\t%s\n' \
    "$(basename "$FILE")" \
    "$COLOR"
}

if [[ "$SYSTEM" == Darwin ]] ; then
  macos::install_deps

  if [[ ! -d "$EXTRACTDIR/assets/minecraft/textures/" ]] ; then
    unpack_assets
  fi
fi

average $1
