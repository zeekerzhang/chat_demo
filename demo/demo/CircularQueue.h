#ifndef CircularQueueH
#define CircularQueueH

#include "stdafx.h"
#include <windows.h>
#include <process.h>

template <typename T>
class CircularQueue
{
public:
    explicit CircularQueue(int capacity) : m_capacity(capacity), m_head(0), m_tail(0)
    {
        m_array = new T[m_capacity + 1];
    }
    
    ~CircularQueue()
    {
        delete[] m_array;
        m_capacity = m_head = m_tail = 0;
    }
    
    bool IsFull()
    {
        int offset = (m_tail + 1) % (m_capacity + 1);
        
        return (offset == m_head);
    }
    
    bool IsEmpty()
    {
        return (m_tail == m_head);
    }
    
    void Push(const T& item)
    {
        if ( !IsFull() )
        {
            m_array[m_tail] = item;
            m_tail = (m_tail + 1) % (m_capacity + 1);
        }
    }
    
    T Pop()
    {
        if ( IsEmpty() )
        {
            return T();
        }
        
        int index = m_head;
        m_head = (m_head + 1) % (m_capacity + 1);
        
        return m_array[index];
    }
    
private:
    T* m_array; // 缓冲区队列
    int m_capacity; // 队列最大存储容量
    int m_head; // 队列头指针
    int m_tail; // 队列尾指针
};
 
// 同步队列
template <typename T>
struct SynQueue
{
    SynQueue(int size) : buffer(size)
    {
        synEventHandle = CreateEvent(NULL, FALSE, FALSE, 0);
    }
 
    ~SynQueue()
    {
        CloseHandle(synEventHandle);
    }

    operator HANDLE()
    {
        return synEventHandle;
    }
 
    CircularQueue<T> buffer; // 缓冲区
    HANDLE synEventHandle; // 同步事件
};
 
class PacketQueue
{
public:
    explicit PacketQueue(int size) : m_readQueue(size), 
        m_sendQueue(size), m_bFreezeQueue(false)
    {
    }
 
    // 刷新读取缓冲区，使得发送线程有机会接管读取缓冲区，在读取完毕后调用
    void Flush()
    {
        SetEvent(m_readQueue);
        SetEvent(m_sendQueue);
    }

    // 冻结缓冲区
    void FreezeQueue()
    {
        m_bFreezeQueue = true;
    }
 
    // 从发送缓冲区取出一个数据包
    char *Popup()
    {
        static SynQueue<char *>* pSendQueue = &m_sendQueue;
        static SynQueue<char *>* pReadQueue = &m_readQueue;

        // 判断发送缓冲区数据包是否为空
        if ( pSendQueue->buffer.IsEmpty() )
        {
            // 释放当前发送缓冲区
            SetEvent(pSendQueue->synEventHandle);

            // 得到当前读取缓冲区
            pReadQueue = ExchangeQueue(pSendQueue);
 
            // 接管当前读取缓冲区的拥有权
            WaitForSingleObject(pReadQueue->synEventHandle, INFINITE);
 
            // 接管读取缓冲区
            pSendQueue = pReadQueue;
        }
         
        // 从缓冲区取出一个数据包
        return pSendQueue->buffer.Pop();
    }
     
    bool Push(char *item)
    {
        static SynQueue<char *>* pReadQueue = &m_readQueue;
        static SynQueue<char *>* pSendQueue = &m_sendQueue;

        if ( m_bFreezeQueue )
        {
            return false;
        }

        // 判断读取缓冲区是否数据已填满
        if ( pReadQueue->buffer.IsFull() )
        {
            // 释放当前缓冲区拥有权
            SetEvent(pReadQueue->synEventHandle);

            pSendQueue = ExchangeQueue(pReadQueue);
 
            // 等待接管另一个缓冲区的拥有权
            WaitForSingleObject(pSendQueue->synEventHandle, INFINITE);
 
            // 接管另一个缓冲区
            pReadQueue = pSendQueue;
        }
         
        // 插入数据包
        pReadQueue->buffer.Push(item);

        return true;
    }
 
private:
    // 交换缓冲区
    SynQueue<char *>* ExchangeQueue(SynQueue<char *>* queue)
    {
        if ( (queue != &m_readQueue) && (queue != &m_sendQueue) )
        {
            return 0;
        }
 
        return (queue == &m_readQueue ? &m_sendQueue : &m_readQueue);
    }
 
private:
    // 双缓冲队列
    SynQueue<char *> m_readQueue; // 数据读取缓冲区
    SynQueue<char *> m_sendQueue; // 数据发送缓冲区

    bool m_bFreezeQueue; // 发送线程出现致命错误,冻结缓冲区
};
#endif
/*
// 读取线程入口函数
unsigned WINAPI ReadThreadEntry( PVOID param )
{
    PacketQueue* queue = (PacketQueue*)param;

    // 插入100个数据包
    for ( int i = 0; i < 10000; ++i )
    {
        Sleep(100); // 读取间隔时间

        if ( !queue->Push(i) )
        {
            goto exit;
        }

        printf("intput data: %d\n", i);
    }

exit:

    // 插入结束包
    queue->Push(-1);
    printf("input end data\n");

    // 刷新读缓冲区
    queue->Flush();
    printf("flush read buffer\n");
     
    return 0;
}
 
// 发送线程入口函数
unsigned WINAPI SendThreadEntry( PVOID param )
{
    PacketQueue* queue = (PacketQueue*)param;

    // 循环读取数据包
    for (;;)
    {
        //queue->FreezeQueue();
        //return 0;

        int item = queue->Popup();
        if ( -1 == item )
        {
            printf("send thread fetch end data\n");
            break; // 遇到结束包
        }

        //Sleep(150);
        printf("send thread fetch data: %d\n", item);
    }
     
    return 0;
}
 
int main(int argc, char* argv[])
{
    PacketQueue queue(50);
 
    // 启动读取线程
    HANDLE hReadThread = (HANDLE)_beginthreadex(NULL, 0, ReadThreadEntry, (void*)&queue, 0, NULL);
 
    // 启动发送线程
    HANDLE hSendThread = (HANDLE)_beginthreadex(NULL, 0, SendThreadEntry, (void*)&queue, 0, NULL);

    DWORD T1 = GetTickCount();

    WaitForSingleObject(hReadThread, INFINITE);
    WaitForSingleObject(hSendThread, INFINITE);

    printf("total time: %d", GetTickCount() - T1);
 
    return 0;
}
*/