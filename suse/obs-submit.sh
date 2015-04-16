#!/bin/bash 
PRJ="$1"
PKG=lpub4

function die() {
	echo "ERROR: $1" >&2
	exit 1
}

function warn() {
	echo "WARNING: $1" >&2
}

#
# Check options and the commit the user wants to make a package from
#
if [[ -z $1 ]] || [[ "$1" == "-h" ]] || [[ "$1" == "--help" ]] ; then
	echo "Usage: $0 OBS_PROJECT_NAME [build|force]" >&2
	exit 1
fi

BUILD=false
FORCE=false
[[ "$2" == "force" ]] && FORCE=true
[[ "$2" == "build" ]] && {
	BUILD=true
	FORCE=true
}

if git status --porcelain |grep -q M; then
	warn "Uncommited changes:"
	git status --short >&2
	if git status --porcelain .. |grep -q M && ! $FORCE; then
		die "Please commit your changes to git first. To override, run: $0 $1 force"
	fi
fi

TAG=`git tag --points-at HEAD`
if [[ -z $TAG ]]; then 
	warn "Not on a tagged commit."
	TAG=`git describe --abbrev=0`
	[[ -z $TAG ]] && exit 1

	if ! $FORCE; then
		die "To use the most recent tag ($TAG) as the version string, run: $0 $1 force"
	fi
fi
echo "Using $TAG as version string."	


#
# Create the directory of package sources
#
D=`mktemp -d`


pushd $D
if ! osc co "$PRJ" "$PKG"; then
	popd
	rm -rf $D
	die "Cannot check the project out from BS."
fi

PRJD="$D/$PRJ/$PKG"

if ! cd $PRJD; then 
	popd
	rm -rf $D
	die "Cannot get into the package directory."
fi
rm -rf *

popd

SPEC=$PKG.spec
sed "s/__VERSION__/$TAG/" $SPEC >$PRJD/$SPEC

pushd ..
git archive --prefix=$PKG/ HEAD | bzip2 > $PRJD/$PKG.tar.bz2
popd
HASH=`git log -1 --pretty="format:%H"`

pushd $PRJD
if $BUILD; then
	osc build
	echo "Keeping build source directory: $D"
else
	osc addremove
	osc vc -m "See the GIT history at https://github.com/ldraw-linux/$PKG/commits/$HASH"
	osc add $PKG.changes
	osc commit -m "git commit: $HASH"
	popd
	rm -rf $D
fi
