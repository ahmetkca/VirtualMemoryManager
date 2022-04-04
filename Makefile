all:
	clang -g -Wall -Wextra -o main.out main.c

clean:
	rm -f main.out