/**
 * ��Э�̿����Է��ƴ��, ����ucontext���ӿڻ���ģ��luaЭ�̣��Ƚ����׶���
 * ��ϸ: http://blog.codingnow.com/2012/07/c_coroutine.html
 *
 * �Ƚϵͼ���Э�̿�(Protothreads)������бȽ���ϸ�Ľ��ܣ�����Э�̿����ߡ�
 * ��ϸ: http://coolshell.cn/articles/10975.html
 */

#ifndef C_COROUTINE_H
#define C_COROUTINE_H

// Э��״̬
#define COROUTINE_DEAD    0
#define COROUTINE_READY   1
#define COROUTINE_RUNNING 2
#define COROUTINE_SUSPEND 3

struct schedule;

typedef void (*coroutine_func)(struct schedule *, void *ud);

// ����Э���{�Ƚṹ
struct schedule * coroutine_open(void);
// ����Э���{�Ƚṹ
void coroutine_close(struct schedule *);

// ����һ���µ�Э��
int coroutine_new(struct schedule *, coroutine_func, void *ud);
// resumeһ��Э��
void coroutine_resume(struct schedule *, int id);
// ��ȡЭ�̵�״̬
int coroutine_status(struct schedule *, int id);
// ��ȡ��ǰ���е�Э��
int coroutine_running(struct schedule *);
// yieldһ��Э��
void coroutine_yield(struct schedule *);

#endif
