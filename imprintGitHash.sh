#!/bin/sh

FILE="$(dirname "$0")/src/git_hash.hpp"
HASH=$(git rev-parse HEAD)
COMMAND="const static char* gitCommitHash =\"$HASH\";"

CURRENT="$(cat "$FILE")"
if [ "$COMMAND" != "$CURRENT" ]
then
    echo "$COMMAND" > "$FILE"
fi