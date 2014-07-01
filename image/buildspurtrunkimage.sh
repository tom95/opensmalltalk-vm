#!/bin/sh
. ./envvars.sh
./getsqueak45.sh
. ./get3021vm.sh

cp -p $SQUEAK45.image trunk46forspur.image
cp -p $SQUEAK45.changes trunk46forspur.changes

rm -f temp-spur-repository/* temp-v3-repository/*
echo $VM trunk46forspur.image BuildSqueakTrunkImage.st
$VM trunk46forspur.image BuildSqueakTrunkImage.st
echo $VM trunk46forspur.image WriteSpurPackagesToTempDir.st
$VM trunk46forspur.image WriteSpurPackagesToTempDir.st

# Now choose a suitable VMMaker image (Spur preferred) and get it to convert
# trunk46forspur to Spur.
IMAGE=""
GOTIMAGE=""
while true; do
	for f in *VMMaker*spur.image; do
		if test -f "$f"; then
			IMAGE="$f"
.			./get3021spurvm.sh
			break 2
		fi
	done
	if [ -z "$IMAGE" ]; then
		for f in *VMMaker*.image; do
			if test -f "$f"; then
				IMAGE="$f"
.				./get3021vm.sh
				break 2
			fi
		done
	fi
	if [ -n "$GOTIMAGE" ]; then
		echo hmmm, failed to build a VMMaker image.  Bailing out
		exit 1
	fi
	if [ -z "$IMAGE" ]; then
		buildsqueaktrunkvmmakerimage.sh
		GOTIMAGE=1
	fi
done
echo $VM $IMAGE BuildSpurTrunkImage.st
$VM $IMAGE BuildSpurTrunkImage.st
mv trunk46forspur-spur.image trunk46-spur.image
mv trunk46forspur-spur.changes trunk46-spur.changes

# Now load the modified packages
. ./get3021spurvm.sh
echo $VM trunk46-spur.image LoadSpurPackagesFromTempDir.st
$VM trunk46-spur.image LoadSpurPackagesFromTempDir.st