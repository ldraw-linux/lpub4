#!/bin/bash 
PKG=lpub4

#
# a block of functions
# look for "main" to skip it
#
function die() {
	echo "ERROR: $1" >&2
	exit 1
}

function warn() {
	echo "WARNING: $1" >&2
}

# This function expects an annotated tag 'start' containing two lines in the messsage:
#   Upstream version of the project at that time
#   URL of upstream
function shorten_history() {
	git cat-file -p start | {
		while true ; do
			read line
			if [ -z "$line" ] ; then
				break
			fi
		done
		read version
		read URL
		git log --pretty=format:"-------------------------------------------------------------------%n%ad - %ce%n%n- ${version}%n  ${URL}%n" start^..start ;
	}
}

function generate_changes_file() {
	git log --pretty=format:'-------------------------------------------------------------------%n%ad - %ce%n%n- %s%n  %h%n' start..HEAD | cat
	shorten_history
}

#
# main
#

#
# Check options and the commit the user wants to make a package from
#
if [[ -z $1 ]] || [[ "$1" == "-h" ]] || [[ "$1" == "--help" ]] ; then
	echo "Usage: $0 [local|OBS_PROJECT_NAME] [build|force]" >&2
	exit 1
fi

LOCAL=false
PRJ=""
BUILD=false
FORCE=false
if [[ "$1" == "local" ]] ; then
	LOCAL=true
else
	PRJ="$1"
fi
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

HASH=`git log -1 --pretty="format:%H"`
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

SRCDIR="$D/$PKG"
mkdir "$SRCDIR" || die "Cannot create a temporary directory $SRCDIR"

SPEC=$PKG.spec
CHANGEFILE=$PKG.changes
git archive --prefix=$PKG/ HEAD | bzip2 > $SRCDIR/$PKG.tar.bz2
sed "s/__VERSION__/$TAG/" suse/$SPEC >$SRCDIR/$SPEC
generate_changes_file >$SRCDIR/$CHANGEFILE

if $LOCAL ; then
	echo "Sources of the package are stored at $SRCDIR" >&2
	exit 0
fi

#
# OSC part
#

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

rm -rf ./*
cp $SRCDIR/* .

if $BUILD; then
	osc build
	echo "Keeping build source directory: $D"
else
	osc addremove
	osc commit -m "git commit: $TAG($HASH)"
	popd
	rm -rf $D
fi
