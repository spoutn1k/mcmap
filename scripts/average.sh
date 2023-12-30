#!/usr/bin/env bash

# Requires ImageMagick
# Outputs the color average of the image passed as an argument

set -eo pipefail

install_imagemagick() {
  if ! command -v convert &>/dev/null; then
    if [[ "${UNAME}" == "Darwin" ]] && command -v brew &>/dev/null; then
      brew install imagemagick
    fi
  fi
}

unpack_minecraft_jar() {
  if [[ -z "${MINECRAFT_VER}" ]]; then
    return
  fi

  if [[ -f "extracted_blocks/${MINECRAFT_VER}/assets/minecraft/textures/block/dirt.png" ]]; then
    return
  fi

  if [[ "${UNAME}" == "Darwin" ]]; then
    jar_file="${HOME}/Library/Application Support/minecraft/versions/${MINECRAFT_VER}/${MINECRAFT_VER}.jar"
  fi

  if [[ -n "${jar_file}" ]]; then
    mkdir -p "extracted_blocks/${MINECRAFT_VER}"
    pushd "extracted_blocks/${MINECRAFT_VER}"
    jar xf "${jar_file}" assets/minecraft/textures/block
    popd
  fi
}

install_imagemagick
unpack_minecraft_jar

printf '%s\t%s\n' \
  "$(basename "extracted_blocks/${MINECRAFT_VER}/assets/minecraft/textures/block/${1}.png")" \
  "$(convert "extracted_blocks/${MINECRAFT_VER}/assets/minecraft/textures/block/${1}.png" -resize 1x1 txt:- \
    | grep -o "#[[:xdigit:]]\{6\}" | tr A-F a-f)"
