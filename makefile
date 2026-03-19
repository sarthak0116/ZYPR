all:
	gcc -o out main.c zip.c inflate.c


clean:
	rm -f out