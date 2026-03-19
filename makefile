all:
	gcc -o unzip src/main.c src/zip.c src/inflate.c -Iinclude

clean:
	rm -f unzip