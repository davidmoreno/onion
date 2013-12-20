#!/bin/sh

SOURCE_DIR=../../..
BUILD_DIR=../..

OPACK=${OPACK:-$BUILD_DIR/tools/opack/opack}
OTEMPLATE=${OTEMPLATE:-$BUILD_DIR/tools/otemplate/otemplate}

ORIG_DIR=${ORIG_DIR:-$SOURCE_DIR/tests/07-tools/}
DEST_DIR=${DEST_DIR:-$BUILD_DIR/tests/07-tools/}


ASSETS_H=$DEST_DIR/assets.h

rm -f $ASSETS_H

echo $OPACK $ORIG_DIR/test-assets.sh -o $DEST_DIR/test_pack.c -a $ASSETS_H

$OPACK $ORIG_DIR/test-assets.sh -o $DEST_DIR/test_pack.c -a $ASSETS_H || exit 1

grep test_sh assets.h || ( echo "No assets.h" && exit 1 )

$OTEMPLATE $ORIG_DIR/external.html $DEST_DIR/test_templates.c -a $ASSETS_H || exit 1
$OTEMPLATE $ORIG_DIR/base.html $DEST_DIR/test_templates.c -a $ASSETS_H || exit 1
$OTEMPLATE $ORIG_DIR/test.html $DEST_DIR/test_templates.c -a $ASSETS_H || exit 1

grep test_sh assets.h || ( echo "No assets.h" && exit 1 )
grep test_html assets.h || ( echo "No assets.h" && exit 1 )
grep external_html assets.h || ( echo "No assets.h" && exit 1 )
grep base_html assets.h || ( echo "No assets.h" && exit 1 )

cat assets.h

rm $ASSETS_H
touch $ASSETS_H
$OPACK $ORIG_DIR/test-assets.sh -o $ORIG_DIR/test_pack.c -a $ASSETS_H || exit 1

grep "#define __ONION_ASSETS_H__" $ASSETS_H || ( echo "Did not create assets file from empty file" || exit 1 )

echo "Twice"

$OPACK $ORIG_DIR/test-assets.sh -o $ORIG_DIR/test_pack.c -a $ASSETS_H || exit 1
[ $( grep "#define __ONION_ASSETS_H__" $ASSETS_H | wc -l ) = 1 ] || ( echo "Did not create assets file from empty file" || exit 1 )

cat assets.h

echo "OK"