#!/bin/bash
# ImageMagick 
#  https://learn.adafruit.com/preparing-graphics-for-e-ink-displays for more info


convert test1.jpg -dither FloydSteinberg -units PixelsPerInch   -define dither:diffusion-amount=85% -resize 600x448 -remap epaper_eink-7color.png -density 96 +profile "*" test1-out.bmp