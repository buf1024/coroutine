all : main exam

main : main.c coroutine.c
	gcc -g -Wall -o $@ $^
exam : exam.c
	gcc -g -Wall -o $@ $^

clean :
	rm main
