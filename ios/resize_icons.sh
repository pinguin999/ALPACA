#!/bin/bash

set -eux

echo OUTDATED! Use Icon Set Creator from the App Store instead.
exit 1

mkdir icons

# https://stackoverflow.com/a/19087045/647898
convert logo1024.png -resize 57x57 icons/Icon.png
convert logo1024.png -resize 114x114 icons/Icon@2x.png
convert logo1024.png -resize 29x29 icons/Icon-Small.png
convert logo1024.png -resize 58x58 icons/Icon-Small@2x.png
convert logo1024.png -resize 40x40 icons/Icon-40.png
convert logo1024.png -resize 80x80 icons/Icon-40@2x.png
convert logo1024.png -resize 120x120 icons/Icon-60@2x.png
convert logo1024.png -resize 76x76 icons/Icon-76.png
convert logo1024.png -resize 152x152 icons/Icon-76@2x.png
