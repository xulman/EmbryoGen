#!/bin/sh

echo "const static char* gitCommitHash = \"$(git rev-parse HEAD)\";" > $(dirname $0)/src/git_hash.hpp
