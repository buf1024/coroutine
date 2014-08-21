#include "coroutine.h"
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>

// stack��С
#define STACK_SIZE (1024*1024)
// Э��Ĭ�ϴ�С
#define DEFAULT_COROUTINE 16

struct coroutine;

// Э�̵��ȴ�С
struct schedule {
	char stack[STACK_SIZE]; // ��ǰ���е�Э�̵�ջ
	ucontext_t main; // �¸�Э���л���������״̬
	int nco; // ��ǰЭ��
	int cap; // ����
	int running; // ��ǰ���е�Э��
	struct coroutine **co; // Э������
};

struct coroutine {
	coroutine_func func; // ���ú���
	void *ud;      // �û�����
	ucontext_t ctx; // �����Э��������״̬
	struct schedule * sch; // ����struct scheduleָ��
	ptrdiff_t cap; // �������л�ʱ�����ջ������
	ptrdiff_t size; // �������л�ʱ�����ջ�Ĵ�С size <= cap
	int status; // Э��״̬
	char *stack; // �����Э��ջ��С
};

// �ڲ������������ⲿ���á�
static struct coroutine *
_co_new(struct schedule *S , coroutine_func func, void *ud) {
	struct coroutine * co = malloc(sizeof(*co));
	co->func = func;
	co->ud = ud;
	co->sch = S;
	co->cap = 0;
	co->size = 0;
	co->status = COROUTINE_READY;
	co->stack = NULL;
	return co;
}

static void
_co_delete(struct coroutine *co) {
	free(co->stack); // free NULL��OK��
	free(co);
}

struct schedule * 
coroutine_open(void) {
	struct schedule *S = malloc(sizeof(*S));
	S->nco = 0;
	S->cap = DEFAULT_COROUTINE;
	S->running = -1;
	S->co = malloc(sizeof(struct coroutine *) * S->cap);
	memset(S->co, 0, sizeof(struct coroutine *) * S->cap);
	return S;
}

void 
coroutine_close(struct schedule *S) {
	int i;
	for (i=0;i<S->cap;i++) {
		struct coroutine * co = S->co[i];
		if (co) {
			_co_delete(co);
		}
	}
	free(S->co);
	S->co = NULL;
	free(S);
}

int 
coroutine_new(struct schedule *S, coroutine_func func, void *ud) {
	struct coroutine *co = _co_new(S, func , ud);
	if (S->nco >= S->cap) {
	    // ��2����ǰ��С�Զ�����
		int id = S->cap;
		S->co = realloc(S->co, S->cap * 2 * sizeof(struct coroutine *));
		memset(S->co + S->cap , 0 , sizeof(struct coroutine *) * S->cap);
		S->co[S->cap] = co;
		S->cap *= 2;
		++S->nco;
		return id;
	} else {
		int i;
		for (i=0;i<S->cap;i++) {
			int id = (i+S->nco) % S->cap;
			if (S->co[id] == NULL) {
				S->co[id] = co;
				++S->nco;
				return id;
			}
		}
	}
	assert(0);
	return -1;
}

// �����������л�ʱ����
static void
mainfunc(uint32_t low32, uint32_t hi32) {
	uintptr_t ptr = (uintptr_t)low32 | ((uintptr_t)hi32 << 32);
	struct schedule *S = (struct schedule *)ptr;
	int id = S->running;
	struct coroutine *C = S->co[id];
	C->func(S,C->ud);
	_co_delete(C); // �������ɾ����Э��
	S->co[id] = NULL;
	--S->nco;
	S->running = -1;
}

void 
coroutine_resume(struct schedule * S, int id) {
	assert(S->running == -1);
	assert(id >=0 && id < S->cap);
	struct coroutine *C = S->co[id];
	if (C == NULL)
		return;
	int status = C->status;
	switch(status) {
	case COROUTINE_READY:
		getcontext(&C->ctx);
		C->ctx.uc_stack.ss_sp = S->stack;
		C->ctx.uc_stack.ss_size = STACK_SIZE;
		C->ctx.uc_link = &S->main; // �¸��л���������״̬����swapcontext������ֵ
		S->running = id;
		C->status = COROUTINE_RUNNING;
		uintptr_t ptr = (uintptr_t)S;
		makecontext(&C->ctx, (void (*)(void)) mainfunc, 2, (uint32_t)ptr, (uint32_t)(ptr>>32));
		swapcontext(&S->main, &C->ctx);
		break;
	case COROUTINE_SUSPEND:
		memcpy(S->stack + STACK_SIZE - C->size, C->stack, C->size);
		S->running = id;
		C->status = COROUTINE_RUNNING;
		swapcontext(&S->main, &C->ctx);
		break;
	default:
		assert(0);
		break;
	}
}

static void
_save_stack(struct coroutine *C, char *top) {
	char dummy = 0;
	//��������һ���ض��ļ���
	// ջ�ɸߵ�ַ��͵�ַ�������������ֳ��򲼾ֵ�CPUһ����X86���ܵ�
	// ���������ʱ��CPU�ṹ��صġ�
	assert(top - &dummy <= STACK_SIZE); // stack��С
	if (C->cap < top - &dummy) {
		free(C->stack); // �״�C->stackΪNULL,free(NULL)��OK��
		C->cap = top-&dummy;
		C->stack = malloc(C->cap);
	}
	C->size = top - &dummy;
	memcpy(C->stack, &dummy, C->size);
}

void
coroutine_yield(struct schedule * S) {
	int id = S->running;
	assert(id >= 0);
	struct coroutine * C = S->co[id];
	assert((char *)&C > S->stack);
	_save_stack(C,S->stack + STACK_SIZE);
	C->status = COROUTINE_SUSPEND;
	S->running = -1;
	// ��д��״̬�л�
	swapcontext(&C->ctx , &S->main);
}

int 
coroutine_status(struct schedule * S, int id) {
	assert(id>=0 && id < S->cap);
	if (S->co[id] == NULL) {
		return COROUTINE_DEAD;
	}
	return S->co[id]->status;
}

int 
coroutine_running(struct schedule * S) {
	return S->running;
}

