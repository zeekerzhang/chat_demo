//---------------------------------------------------------------------------

#ifndef LockFreeQueueH
#define LockFreeQueueH
//---------------------------------------------------------------------------
#include <windows.h>
#include "TypeDef.h"

namespace lock_free
{
	//typedef int                 int32;
	//typedef unsigned int        uint32;

	//原子加
	inline uint32 AtomAdd(void * var, const uint32 value)
	{
		return InterlockedExchangeAdd((long *)(var), value); // NOLINT
	}

	//原子减
	inline uint32 AtomDec(void * var, int32 value)
	{
		value = value * -1;
		return InterlockedExchangeAdd((long *)(var), value); // NOLINT
	}

	class Queue
	{
		typedef struct QUEUE_NODE
		{
			bool IsEmpty;
			int32 writting;
			int32 reading;
			void *pObject;
		}QUEUE_NODE;

	public:
		Queue( int32 nSize );
		virtual ~Queue();

	public:
		bool Push( void *pObject );
		void* Pop();
		void Clear();//清除数据
	protected:

	private:
		QUEUE_NODE *m_queue;
		uint32 m_push;
		uint32 m_pop;
		uint32 m_nSize;
		int32 m_nWriteAbleCount;
		int32 m_nReadAbleCount;
	};
}
# endif
