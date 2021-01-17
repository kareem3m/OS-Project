build:
	gcc -o process_generator process_generator.c -lm
	gcc -o clk clk.c -lm
	gcc -o process process.c -lm
	gcc -o test_generator test_generator.c -lm
	gcc -o HPF HPF.c -lm
	gcc -o SRTN SRTN.c -lm
	gcc -o RR RR.c -lm


clean:
	rm -f *.out  processes.txt

all: clean build

run:
	./process_generator
