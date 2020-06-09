#!/bin/sh 
cd $(dirname $0)

OPK_NAME=mGBA-SX.opk

echo Building ${OPK_NAME}...

# create opk
FLIST="../sdl/mgba bios.bin font.ttf default.gcw0.desktop mgbasx.png bg.png manual-en.txt"
#if [ -f ../translations/locales ]; then
#  FLIST="${FLIST} ../translations/locales"
#  if [ -d ../build/locale ]; then
#    FLIST="${FLIST} ../build/locale"
#  fi
#fi

rm -f ${OPK_NAME}
mksquashfs ${FLIST} ${OPK_NAME} -all-root -no-xattrs -noappend -no-exports


cat default.gcw0.desktop
