build:
	gcc producer.c -o producer.out
	gcc consumer.c -o consumer.out

clean:
	rm -f *.out

all: clean build

run1:
	./producer.out 16

run2:
	./consumer.out