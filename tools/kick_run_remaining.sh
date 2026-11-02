#!/bin/sh
set -eu
sh tools/kick_resume_required.sh >build/kick-remaining.log 2>&1
printf "KICK_REMAINING_PASS\n"
