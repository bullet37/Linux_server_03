/*
线程为消费者，任务队列为生产者
任务队列
工作线程
管理者线程
Author: tz116
Date:S 2021-11-16
*/

#ifndef __THREADPOOL__
#define __THREADPOOL__
typedef struct ThreadPool ThreadPool;
ThreadPool* threadPoolCreate(int min, int max, int queueSize);	// 创建线程池并初始化
int threadPoolDestroy(ThreadPool* pool);	// 销毁线程池
void threadPoolAdd(ThreadPool* pool, void(*func)(void*), void* arg);	// 给线程池添加任务
int threadPoolBusyNum(ThreadPool* pool);	// 获取线程池中工作的线程的个数
int threadPoolAliveNum(ThreadPool* pool);	// 获取线程池中活着的线程的个数

void* worker(void* arg);
void* manager(void* arg);
void threadExit(ThreadPool* pool); // 单个线程退出
#endif 
