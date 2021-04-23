compare: compare.c
	gcc -g -std=c99 -Wvla -Wall -fsanitize=address,undefined compare.c -lm -o compare -pthread

dcompare: compare.c
	gcc -g -std=c99 -Wvla -Wall -fsanitize=address,undefined -DDEBUG compare.c -lm -o compare -pthread 
