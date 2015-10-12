//---------------------------------------------------------------------------

#include "stdafx.h"
#pragma hdrstop

#include "LockFreeQueue.h"

//---------------------------------------------------------------------------
#ifndef NULL
#define NULL 0
#endif
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
namespace lock_free
{
	Queue::Queue( int32 nSize )
	{
		m_nSize = nSize;
		m_nWriteAbleCount = m_nSize;
		m_nReadAbleCount = 0;
		m_queue = new QUEUE_NODE[m_nSize];
		m_push = 0;
		m_pop = 0;
		Clear();
	}

	Queue::~Queue()
	{
		if ( NULL == m_queue ) return;
		delete[]m_queue;
	}

	bool Queue::Push( void *pObject )
	{
		if ( 0 >= m_nWriteAbleCount ) return false;//队列已满
		if ( 0 >= AtomDec(&m_nWriteAbleCount,1) ) //最多允许m_nWriteAbleCount多个push线程操作队列，确保之后遍历的操作一定能够找到空位插入
		{
			AtomAdd(&m_nWriteAbleCount, 1);
			return false;
		}

		uint32 pushPos = m_push;
		AtomAdd(&m_push, 1);
		uint32 i = 0;
		for ( i = 0; i < m_nSize; i++, pushPos++ )
		{
			pushPos = pushPos % m_nSize;
			if ( !m_queue[pushPos].IsEmpty ) continue;
			if ( 0 < AtomAdd(&m_queue[pushPos].writting, 1) ) continue;
			break;
		}

		m_queue[pushPos].pObject = pObject;
		m_queue[pushPos].IsEmpty = false;
		m_queue[pushPos].reading = 0;
		AtomAdd(&m_nReadAbleCount,1);

		return true;
	}

	void* Queue::Pop()
	{
		if ( 0 >= m_nReadAbleCount ) return NULL;//空队列
		if ( 0 >= AtomDec(&m_nReadAbleCount,1)) //最多允许m_nReadAbleCount多个Pop线程操作队列，确保之后遍历的操作一定能够找到数据取出
		{
			AtomAdd(&m_nReadAbleCount, 1);
			return NULL;
		}

		uint32 popPos = m_pop;
		AtomAdd(&m_pop, 1);
		uint32 i = 0;
		for ( i = 0; i < m_nSize; i++, popPos++ )
		{
			popPos = popPos % m_nSize;
			if ( m_queue[popPos].IsEmpty ) continue;
			if ( 0 < AtomAdd(&m_queue[popPos].reading, 1) ) continue;
			break;
		}

		void *pObject = m_queue[popPos].pObject;
		m_queue[popPos].pObject = NULL;
		m_queue[popPos].IsEmpty = true;
		m_queue[popPos].writting = 0;
		AtomAdd(&m_nWriteAbleCount,1);

		return pObject;
	}

	void Queue::Clear()
	{
		if ( NULL == m_queue ) return;
		uint32 i = 0;
		m_nWriteAbleCount = m_nSize;
		m_nReadAbleCount = 0;
		for ( i = 0; i < m_nSize; i++ )
		{
			m_queue[i].IsEmpty = true;
			m_queue[i].writting = 0;
			m_queue[i].reading = 0;
			m_queue[i].pObject = NULL;
		}
	}
}