all:
	mkdir -p build
	gcc -o build/updater main.c -lwsock32 -mwindows

.PHONY: clean
clean:
	rm -f **/*.o