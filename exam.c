/*
 * exam.c
 *
 *  Created on: 2016年4月28日
 *      Author: heidong
 */

#include <ucontext.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#define MIN_STACK_SIZE  (1024*8)

typedef void (*sigfunc_t)(int);
typedef struct coro_s coro_t;
typedef int (*coro_fun_t)(coro_t*);

struct coro_s
{
	ucontext_t callee, caller;
	coro_fun_t func;
	int yield;
};
coro_t* co_new(coro_fun_t func);
int co_resume(coro_t* co);
int co_yield(coro_t* co);

int co_fun(coro_t* co, coro_fun_t func)
{
	int v = func(co);
	co->yield = v;
	co_yield(co);
	return 0;
}

coro_t* co_new(coro_fun_t func)
{
	coro_t* co = malloc(sizeof(*co) + MIN_STACK_SIZE);

	getcontext(&co->callee);

	unsigned char* stack = (unsigned char*)(co + 1);

	co->callee.uc_stack.ss_flags = 0;
	co->callee.uc_stack.ss_sp = stack;
	co->callee.uc_stack.ss_size = MIN_STACK_SIZE;
	co->callee.uc_link = NULL;

	makecontext(&co->callee, (void(*)())co_fun, 2, co, func);

	return co;
}
int co_resume(coro_t* co)
{
	swapcontext(&co->caller, &co->callee);
	return 0;
}
int co_yield(coro_t* co)
{
	swapcontext(&co->callee, &co->caller);
	return co->yield;
}



int test_func(coro_t* co)
{
	int n = 0;
	while(1) {
		printf("test_func: %d\n", n++);
		co_yield(co);
	}

	return 0;
}

sigfunc_t reg_signal(int signo, sigfunc_t func)
{
    struct sigaction act, oact;
    act.sa_handler = func;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    if (sigaction(signo, &act, &oact) < 0)
        return SIG_ERR;

    return oact.sa_handler;
}
void quit(int signo) {
	printf("signal int\n");
	exit(0);
}
int main(int argc, char **argv) {
    printf("main\n");

    reg_signal(SIGINT, quit);
    coro_t* co = co_new(test_func);
    int n = 0;
    while(1) {
    	printf("main fun: %d\n", n++);
    	co_resume(co);
    	sleep(1);
    }
    return 0;
}

