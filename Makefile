
all: output

run:
	./myShell

output: myShell.o parser.o
	gcc myShell.o parser.o -o myShell

myShell.o: myShell.c
	gcc -c myShell.c

parser.o: parser.c parser.h
	gcc -c parser.c

clean:
	rm *.o myShell
