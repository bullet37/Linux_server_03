/*
�߳�Ϊ�����ߣ��������Ϊ������
�������
�����߳�
�������߳�
Author: tz116
Date:S 2021-11-16
*/

#ifndef __THREADPOOL__
#define __THREADPOOL__
typedef struct ThreadPool ThreadPool;
ThreadPool* threadPoolCreate(int min, int max, int queueSize);	// �����̳߳ز���ʼ��
int threadPoolDestroy(ThreadPool* pool);	// �����̳߳�
void threadPoolAdd(ThreadPool* pool, void(*func)(void*), void* arg);	// ���̳߳��������
int threadPoolBusyNum(ThreadPool* pool);	// ��ȡ�̳߳��й������̵߳ĸ���
int threadPoolAliveNum(ThreadPool* pool);	// ��ȡ�̳߳��л��ŵ��̵߳ĸ���

void* worker(void* arg);
void* manager(void* arg);
void threadExit(ThreadPool* pool); // �����߳��˳�
#endif 
