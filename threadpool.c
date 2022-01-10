#include"threadpool.h"
#include<pthread.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<stdio.h>
#define NUMBER 2
//任务
typedef struct Task {
	void(*function)(void*);
	void* arg;
}Task;

//线程池
struct ThreadPool {
	Task* taskQue;	//任务队列
	int queueCapacity;	// 容量
	int queueSize;		// 任务数量
	int queueFront;		// 对头，取数
	int queueRear;		//队尾，放数
	 
	pthread_t managerID;	//管理者线程ID
	pthread_t* threadIDs;	// 工作线程ID
	int minNum;		// 最小线程数
	int maxNum;			
	int busyNum;	// 忙线程个数
	int liveNum;	// 存活线程
	int exitNum;	// 需要销毁的线程数量
	pthread_mutex_t mutexPool;	// 线程池的锁
	pthread_mutex_t mutexBusy;	//锁住Busy变量
	pthread_cond_t notFull;
	pthread_cond_t notEmpty;	// 条件变量判断是否需要阻塞

	int shutdown;	//是否需要销毁线程池
};

ThreadPool* threadPoolCreate(int min, int max, int queueSize) {
	ThreadPool* pool = (ThreadPool*)malloc(sizeof(ThreadPool));
	
	do {
		if (pool == NULL) {
			printf("ThreadPool malloc fail\n");
			break;
		}
		pool->threadIDs = (pthread_t*)malloc(sizeof(pthread_t) * max);
		if (pool->threadIDs == NULL) {
			printf("ThreadIDs malloc fail\n");
			break;
		}
		memset(pool->threadIDs, 0, sizeof(pthread_t) * max);
		pool->minNum = min;
		pool->maxNum = max;
		pool->busyNum = 0;
		pool->exitNum = 0;
		pool->liveNum = min;

		if (pthread_mutex_init(&pool->mutexPool, NULL) != 0 ||
			pthread_mutex_init(&pool->mutexBusy, NULL) != 0 ||
			pthread_cond_init(&pool->notFull, NULL) != 0 ||
			pthread_cond_init(&pool->notEmpty, NULL) != 0
			) {
			printf("cond or mutex init error\n");
			break;
		}
		// 任务队列
		pool->taskQue = (Task*)malloc(sizeof(Task) * queueSize);
		pool->queueCapacity = queueSize;
		pool->queueFront = 0;
		pool->queueRear = 0;
		pool->queueSize = 0;
		pool->shutdown = 0;

		// 创建管理者线程
		pthread_create(&pool->managerID, NULL, manager, pool);
		// 创建工作线程
		for (int i = 0; i < min; ++i) {
			pthread_create(&pool->threadIDs[i], NULL, worker, pool);
		}
		return pool;
	} while (0);
	// 释放资源
	if (pool && pool->threadIDs) free(pool->threadIDs);
	if (pool && pool->taskQue) free(pool->taskQue);
	if (pool) free(pool);
	return NULL;
}
int threadPoolDestroy(ThreadPool* pool)
{
	if (pool == NULL)
	{
		return -1;
	}

	// 关闭线程池
	pool->shutdown = 1;
	// 阻塞回收管理者线程
	pthread_join(pool->managerID, NULL);
	// 唤醒阻塞的消费者线程
	for (int i = 0; i < pool->liveNum; ++i)
	{
		pthread_cond_signal(&pool->notEmpty);
	}
	// 释放堆内存
	if (pool->taskQue)
	{
		free(pool->taskQue);
	}
	if (pool->threadIDs)
	{
		free(pool->threadIDs);
	}

	pthread_mutex_destroy(&pool->mutexPool);
	pthread_mutex_destroy(&pool->mutexBusy);
	pthread_cond_destroy(&pool->notEmpty);
	pthread_cond_destroy(&pool->notFull);

	free(pool);
	pool = NULL;

	return 0;
}


void threadPoolAdd(ThreadPool* pool, void(*func)(void*), void* arg)
{
	pthread_mutex_lock(&pool->mutexPool);
	while (pool->queueSize == pool->queueCapacity && !pool->shutdown)
	{
		// 阻塞生产者线程
		pthread_cond_wait(&pool->notFull, &pool->mutexPool);
	}
	if (pool->shutdown)
	{
		pthread_mutex_unlock(&pool->mutexPool);
		return;
	}
	// 添加任务
	pool->taskQue[pool->queueRear].function = func;
	pool->taskQue[pool->queueRear].arg = arg;
	pool->queueRear = (pool->queueRear + 1) % pool->queueCapacity;
	pool->queueSize++;

	pthread_cond_signal(&pool->notEmpty);
	pthread_mutex_unlock(&pool->mutexPool);
}

int threadPoolBusyNum(ThreadPool* pool)
{
	pthread_mutex_lock(&pool->mutexBusy);
	int busyNum = pool->busyNum;
	pthread_mutex_unlock(&pool->mutexBusy);
	return busyNum;
}

int threadPoolAliveNum(ThreadPool* pool)
{
	pthread_mutex_lock(&pool->mutexPool);
	int aliveNum = pool->liveNum;
	pthread_mutex_unlock(&pool->mutexPool);
	return aliveNum;
}

void* manager(void* arg)
{
	ThreadPool* pool = (ThreadPool*)arg;
	while (!pool->shutdown)
	{
		// 每隔3s检测一次
		sleep(3);
		// 取出线程池中任务的数量和当前线程的数量
		pthread_mutex_lock(&pool->mutexPool);
		int queueSize = pool->queueSize;
		int liveNum = pool->liveNum;
		pthread_mutex_unlock(&pool->mutexPool);

		// 取出忙的线程的数量
		pthread_mutex_lock(&pool->mutexBusy);
		int busyNum = pool->busyNum;
		pthread_mutex_unlock(&pool->mutexBusy);

		// 添加线程
		// 任务的个数>存活的线程个数 && 存活的线程数<最大线程数
		if (queueSize > liveNum && liveNum < pool->maxNum)
		{
			pthread_mutex_lock(&pool->mutexPool);
			int counter = 0;

			for (int i = 0; i < pool->maxNum && counter < NUMBER
				&& pool->liveNum < pool->maxNum; ++i)
			{
				if (pool->threadIDs[i] == 0)
				{
					pthread_create(&pool->threadIDs[i], NULL, worker, pool);
					counter++;
					pool->liveNum++;
				}
			}

			pthread_mutex_unlock(&pool->mutexPool);
		}
		// 销毁线程
		// 忙的线程*2 < 存活的线程数 && 存活的线程>最小线程数
		if (busyNum * 2 < liveNum && liveNum > pool->minNum)
		{
			pthread_mutex_lock(&pool->mutexPool);

			pool->exitNum = NUMBER;

			pthread_mutex_unlock(&pool->mutexPool);
			// 让工作的线程自杀
			for (int i = 0; i < NUMBER; ++i)
			{
				pthread_cond_signal(&pool->notEmpty);
			}
		}

	}
	return NULL;
}

// worker ,消费者，需要一直监听
void* worker(void* arg)
{
	ThreadPool* pool = (ThreadPool*)arg;

	while (1)
	{
		pthread_mutex_lock(&pool->mutexPool);
		// 当前任务队列是否为空
		while (pool->queueSize == 0 && !pool->shutdown)
		{
			// 阻塞工作线程
			pthread_cond_wait(&pool->notEmpty, &pool->mutexPool);

			// 判断是不是要销毁线程
			if (pool->exitNum > 0){
				pool->exitNum--;
				if (pool->liveNum > pool->minNum)
				{
					pool->liveNum--;
					pthread_mutex_unlock(&pool->mutexPool);
					threadExit(pool);
				}
			}
		}

		// 判断线程池是否被关闭了
		if (pool->shutdown)
		{
			pthread_mutex_unlock(&pool->mutexPool);
			threadExit(pool);
		}

		// 从任务队列中取出一个任务
		Task task;
		task.function = pool->taskQue[pool->queueFront].function;
		task.arg = pool->taskQue[pool->queueFront].arg;
		// 移动头结点
		pool->queueFront = (pool->queueFront + 1) % pool->queueCapacity;
		pool->queueSize--;
		// 解锁
		pthread_cond_signal(&pool->notFull);
		pthread_mutex_unlock(&pool->mutexPool);

		printf("thread %ld start working...\n", pthread_self());
		pthread_mutex_lock(&pool->mutexBusy);
		pool->busyNum++;
		pthread_mutex_unlock(&pool->mutexBusy);

		task.function(task.arg);
		free(task.arg);
		task.arg = NULL;

		printf("thread %ld end working...\n", pthread_self());
		pthread_mutex_lock(&pool->mutexBusy);
		pool->busyNum--;
		pthread_mutex_unlock(&pool->mutexBusy);
	}
	return NULL;
}

// 退出当前正在运行的线程
void threadExit(ThreadPool* pool)
{
	pthread_t tid = pthread_self();
	for (int i = 0; i < pool->maxNum; ++i)
	{
		if (pool->threadIDs[i] == tid)
		{
			pool->threadIDs[i] = 0;
			printf("threadExit() called, %ld exiting...\n", tid);
			break;
		}
	}
	pthread_exit(NULL);
}

void taskFunc(void* arg)
{
	int num = *(int*)arg;
	printf("thread %ld is working, number = %d\n",
		pthread_self(), num);
	sleep(1);
}

int main()
{
	// 创建线程池
	ThreadPool* pool = threadPoolCreate(3, 10, 100);
	for (int i = 0; i < 100; ++i)
	{
		int* num = (int*)malloc(sizeof(int));
		*num = i + 100;
		threadPoolAdd(pool, taskFunc, num);
	}

	sleep(30);

	threadPoolDestroy(pool);
	return 0;
}
