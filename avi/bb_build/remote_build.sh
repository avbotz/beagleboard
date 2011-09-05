#!/bin/sh

# Simple rsync "driver" script.  (Uses SSH as the transport layer.)
# http://www.scrounge.org/linux/rsync.html

# Demonstrates how to use rsync to back up a directory tree from a local
# machine to a remote machine.  Then re-run the script, as needed, to keep
# the two machines "in sync."  It only copies new or changed files and ignores
# identical files.

# Destination host machine name
DEST="192.168.2.3"

# User that rsync will connect as
# Are you sure that you want to run as root, though?
USER="root"

# Directory to copy from on the source machine.
BACKDIR="."

# Directory to copy to on the destination machine.
DESTDIR="/home/root/AVBotz/BeagleBoard/code/"

# excludes file - Contains wildcard patterns of files to exclude.
# i.e., *~, *.bak, etc.  One "pattern" per line.
# You must create this file.
EXCLUDES="./bb_build/excludes"

# Options.
# -n Don't do any copying, but display what rsync *would* copy. For testing.
# -a Archive. Mainly propogate file permissions, ownership, timestamp, etc.
# -u Update. Don't copy file if file on destination is newer.
# -v Verbose -vv More verbose. -vvv Even more verbose.
# See man rsync for other options.

# For testing.  Only displays what rsync *would* do and does no actual copying.
#OPTS="-n -vv -u -a --rsh=ssh --exclude-from=$EXCLUDES --stats --progress"
# Does copy, but still gives a verbose display of what it is doing
#OPTS="-v -u -a --rsh=ssh --exclude-from=$EXCLUDES --stats"
# Copies and does no display at all.
OPTS="--archive --update --rsh=ssh --exclude-from=$EXCLUDES --quiet"

# May be needed if run by cron?
#export PATH=$PATH:/bin:/usr/bin:/usr/local/bin

# Only run rsync if $DEST responds.
VAR=`ping -s 1 -c 1 $DEST > /dev/null; echo $?`
if [ $VAR -eq 0 ]; then
	rsync $OPTS $BACKDIR $USER@$DEST:$DESTDIR
	ssh $USER@$DEST "cd "$DESTDIR"; make all"
else
	echo "Cannot connect to $DEST."
fi

