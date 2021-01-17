build:
	gcc process_generator.c -o process_generator -lm
	gcc clk.c -o clk -lm
	gcc RR.c -o RR -lm
	gcc SRTN.c -o SRTN -lm
	gcc HPF.c -o HPF -lm
	gcc process.c -o process -lm
	gcc test_generator.c -o test_generator -lm

clean:
	rm -f *.out  processes.txt

all: clean build

run:
	./process_generator.out
