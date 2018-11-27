all: r

r: client.o  ../common.o ../line_parser.o
	gcc -g -Wall -o r client.o ../common.o ../line_parser.o
	
client.o: client.c
	gcc -g -Wall -c -o client.o client.c
../common.o: ../common.c ../common.h
	gcc -g -Wall -c -o ../common.o ../common.c
../line_parser.o: ../line_parser.c ../line_parser.h
	gcc -g -Wall -c -o ../line_parser.o ../line_parser.c
clean:
	rm -f *.o r 
