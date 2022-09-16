all:
	make clean
	valgrind --leak-check=full \
    --show-leak-kinds=all \
    --track-origins=yes \
    --verbose \
    --log-file=valg.txt \
    ../embryogen_dbgO0 randomDrosophila > out.txt

clean:
	rm -f *.tif
	rm -f *.txt




