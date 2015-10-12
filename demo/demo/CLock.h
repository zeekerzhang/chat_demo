/*********************************/

#ifndef CLOCK_H
#define CLOCK_H

#ifdef _WIN32
#include <Windows.h>
#else
#include <pthread.h>
#endif

#ifdef _WIN32
#define LOCK_MUTEXT CRITICAL_SECTION
#else
#define LOCK_MUTEXT pthread_mutex_t
#endif
class CThreadLock
{
public:
	CThreadLock(void){
		Init();
	}
	~CThreadLock(){
		Close();
	}
	void Lock(){
#ifdef _WIN32
		EnterCriticalSection(&m_lock);
#else
		pthread_mutex_lock(&m_lock);
#endif
	}
	void UnLock(){
#ifdef _WIN32
		LeaveCriticalSection(&m_lock);
#else
		pthread_mutex_unlock(&m_lock);
#endif
	}
//protected:
private:
	//
#ifdef _WIN32
    CRITICAL_SECTION m_lock;
#else
    LOCK_MUTEXT m_lock;
#endif
	void Init(){
#ifdef _WIN32
        InitializeCriticalSection(&m_lock);
#else
		pthread_mutex_init(&m_lock);
#endif
	}
	void Close(){
#ifdef _WIN32
		DeleteCriticalSection(&m_lock);
#else
		pthread_mutex_destroy(&m_lock);
#endif
	}
};

//定义一个自动加锁的类
class CAutoLock
{
public:
	CAutoLock(CThreadLock *pThreadLock){
		m_pThreadLock=pThreadLock;
		if (NULL!=m_pThreadLock)
		{
			m_pThreadLock->Lock();
		}
	}
	~CAutoLock(){
		if (NULL!=m_pThreadLock)
		{
			m_pThreadLock->UnLock();
		}
	}
private:
	CThreadLock * m_pThreadLock;
};
/*********************************/
#endif