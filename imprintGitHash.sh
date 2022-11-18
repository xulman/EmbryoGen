#!/bin/sh

FILE="$(dirname "$0")/src/generated/git_hash.hpp"
HASH=$(git rev-parse HEAD)
COMMAND="const static char* gitCommitHash =\"$HASH\";"

# avoid updating the "hash file" when not necessary,
# to prevent from unnecessary src tree rebuild...
CURRENT="$(cat "$FILE")"
if [ "$COMMAND" != "$CURRENT" ]
then
    echo "$COMMAND" > "$FILE"
fi
