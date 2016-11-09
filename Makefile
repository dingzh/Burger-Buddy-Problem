
bbc: BBC.c
	gcc BBC.c -o $@ -pthread
.PHONY:clean
clean:
	rm bbc
