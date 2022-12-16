websv: websv.o httpf.o
	gcc -o websv websv.o httpf.o
websv.o: websv.c
	gcc -c websv.c
httpf.o: httpf.c
	gcc -c httpf.c

clean:
	rm -f *.o
	rm -f websv
