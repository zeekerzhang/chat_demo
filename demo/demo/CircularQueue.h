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
    T* m_array; // ����������
    int m_capacity; // �������洢����
    int m_head; // ����ͷָ��
    int m_tail; // ����βָ��
};
 
// ͬ������
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
 
    CircularQueue<T> buffer; // ������
    HANDLE synEventHandle; // ͬ���¼�
};
 
class PacketQueue
{
public:
    explicit PacketQueue(int size) : m_readQueue(size), 
        m_sendQueue(size), m_bFreezeQueue(false)
    {
    }
 
    // ˢ�¶�ȡ��������ʹ�÷����߳��л���ӹܶ�ȡ���������ڶ�ȡ��Ϻ����
    void Flush()
    {
        SetEvent(m_readQueue);
        SetEvent(m_sendQueue);
    }

    // ���Ỻ����
    void FreezeQueue()
    {
        m_bFreezeQueue = true;
    }
 
    // �ӷ��ͻ�����ȡ��һ�����ݰ�
    char *Popup()
    {
        static SynQueue<char *>* pSendQueue = &m_sendQueue;
        static SynQueue<char *>* pReadQueue = &m_readQueue;

        // �жϷ��ͻ��������ݰ��Ƿ�Ϊ��
        if ( pSendQueue->buffer.IsEmpty() )
        {
            // �ͷŵ�ǰ���ͻ�����
            SetEvent(pSendQueue->synEventHandle);

            // �õ���ǰ��ȡ������
            pReadQueue = ExchangeQueue(pSendQueue);
 
            // �ӹܵ�ǰ��ȡ��������ӵ��Ȩ
            WaitForSingleObject(pReadQueue->synEventHandle, INFINITE);
 
            // �ӹܶ�ȡ������
            pSendQueue = pReadQueue;
        }
         
        // �ӻ�����ȡ��һ�����ݰ�
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

        // �ж϶�ȡ�������Ƿ�����������
        if ( pReadQueue->buffer.IsFull() )
        {
            // �ͷŵ�ǰ������ӵ��Ȩ
            SetEvent(pReadQueue->synEventHandle);

            pSendQueue = ExchangeQueue(pReadQueue);
 
            // �ȴ��ӹ���һ����������ӵ��Ȩ
            WaitForSingleObject(pSendQueue->synEventHandle, INFINITE);
 
            // �ӹ���һ��������
            pReadQueue = pSendQueue;
        }
         
        // �������ݰ�
        pReadQueue->buffer.Push(item);

        return true;
    }
 
private:
    // ����������
    SynQueue<char *>* ExchangeQueue(SynQueue<char *>* queue)
    {
        if ( (queue != &m_readQueue) && (queue != &m_sendQueue) )
        {
            return 0;
        }
 
        return (queue == &m_readQueue ? &m_sendQueue : &m_readQueue);
    }
 
private:
    // ˫�������
    SynQueue<char *> m_readQueue; // ���ݶ�ȡ������
    SynQueue<char *> m_sendQueue; // ���ݷ��ͻ�����

    bool m_bFreezeQueue; // �����̳߳�����������,���Ỻ����
};
#endif
/*
// ��ȡ�߳���ں���
unsigned WINAPI ReadThreadEntry( PVOID param )
{
    PacketQueue* queue = (PacketQueue*)param;

    // ����100�����ݰ�
    for ( int i = 0; i < 10000; ++i )
    {
        Sleep(100); // ��ȡ���ʱ��

        if ( !queue->Push(i) )
        {
            goto exit;
        }

        printf("intput data: %d\n", i);
    }

exit:

    // ���������
    queue->Push(-1);
    printf("input end data\n");

    // ˢ�¶�������
    queue->Flush();
    printf("flush read buffer\n");
     
    return 0;
}
 
// �����߳���ں���
unsigned WINAPI SendThreadEntry( PVOID param )
{
    PacketQueue* queue = (PacketQueue*)param;

    // ѭ����ȡ���ݰ�
    for (;;)
    {
        //queue->FreezeQueue();
        //return 0;

        int item = queue->Popup();
        if ( -1 == item )
        {
            printf("send thread fetch end data\n");
            break; // ����������
        }

        //Sleep(150);
        printf("send thread fetch data: %d\n", item);
    }
     
    return 0;
}
 
int main(int argc, char* argv[])
{
    PacketQueue queue(50);
 
    // ������ȡ�߳�
    HANDLE hReadThread = (HANDLE)_beginthreadex(NULL, 0, ReadThreadEntry, (void*)&queue, 0, NULL);
 
    // ���������߳�
    HANDLE hSendThread = (HANDLE)_beginthreadex(NULL, 0, SendThreadEntry, (void*)&queue, 0, NULL);

    DWORD T1 = GetTickCount();

    WaitForSingleObject(hReadThread, INFINITE);
    WaitForSingleObject(hSendThread, INFINITE);

    printf("total time: %d", GetTickCount() - T1);
 
    return 0;
}
*/