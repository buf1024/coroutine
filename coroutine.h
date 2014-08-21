/**
 * 本协程库来自风云大哥, 基于ucontext，接口基本模仿lua协程，比较容易懂。
 * 详细: http://blog.codingnow.com/2012/07/c_coroutine.html
 *
 * 比较低级的协程库(Protothreads)，酷壳有比较详细的介绍，包括协程库作者。
 * 详细: http://coolshell.cn/articles/10975.html
 */

#ifndef C_COROUTINE_H
#define C_COROUTINE_H

// 协程状态
#define COROUTINE_DEAD    0
#define COROUTINE_READY   1
#define COROUTINE_RUNNING 2
#define COROUTINE_SUSPEND 3

struct schedule;

typedef void (*coroutine_func)(struct schedule *, void *ud);

// 创建协程調度结构
struct schedule * coroutine_open(void);
// 销毁协程調度结构
void coroutine_close(struct schedule *);

// 创建一个新的协程
int coroutine_new(struct schedule *, coroutine_func, void *ud);
// resume一个协程
void coroutine_resume(struct schedule *, int id);
// 获取协程的状态
int coroutine_status(struct schedule *, int id);
// 获取当前运行的协程
int coroutine_running(struct schedule *);
// yield一个协程
void coroutine_yield(struct schedule *);

#endif
