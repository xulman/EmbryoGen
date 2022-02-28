#! /bin/bash
cd ../src/
FILES=`find . -type f -name "*.cpp" -or -name "*.hpp" -or -name "*.h" | grep -vx '\./tests.*'`
echo "FOUND `echo $FILES | wc -w` files"
python ../docs/dependency_graph.py -o ../docs/graph.json $FILES
cd ../docs/