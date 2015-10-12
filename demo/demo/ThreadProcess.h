//---------------------------------------------------------------------------

#ifndef ThreadProcessH
#define ThreadProcessH
//无锁队列
#include "LockFreeQueue.h"
//内存池
#include "MemPool.h"
//线程池
#include "zthread/Runnable.h"  
#include "zthread/Thread.h"
#include "zthread/PoolExecutor.h" 

#pragma warning(disable:4996)
#pragma comment(lib, "ZThread.lib")
using namespace ZThread;

#define DATA_BLOCK_LEN 1024 
CMemPool myPool1(DATA_BLOCK_LEN, 0, 1024);  

lock_free::Queue rcv_queue(10240);
//---------------------------------------------------------------------------
/*
//生产者线程类
class ProducerProcess : public Runnable  
{  
private:  
	//给任务一个识别id 
    int _id;  
	//计数器
    int _num;                    
public:  
    ProducerProcess(int id) 
		: _id(id)
	{  
        _num = 0;               
    }  
	//实现run函数 
    void run()                 
    {  
		long i = 0;
		while(!Thread::interrupted())
		{				
			Sleep(1);
			++i;
			char *buff = (char *)myPool1.Get();		
			sprintf(buff, "%ld", i);
			queue.Push(buff);
		}
    }  
}; 

//消费者线程类
class CustomerProcess : public Runnable  
{  
//private:  
//    int _id;  
//    int _num;                    
public:  
    CustomerProcess(int id) 
	//	: _id(id)
	{  
    //    _num = 0;               
    }  
	//实现run函数 
    void run()                 
    {  
		while(!Thread::interrupted())
		{				
			Sleep(1);
			char *buff = (char *)queue.Pop();
			if(!buff)
			{
				continue;
			}
			printf("%s\n", buff);
			myPool1.Release(buff);
		}
    }  
};
*/
#define CREATE_PROCESS_OBJ(obj)  \
	class obj : public Runnable  \
	{	\
		public:		\
			obj()	\
			{	\
			}	\
			void run();	\
	};

#define DEFINE_PROCESS_OBJ_FUN(obj) \
	void obj::run()  

#endif