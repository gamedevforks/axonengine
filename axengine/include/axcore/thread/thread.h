/*
Copyright (c) 2009 AxonEngine Team

This file is part of the AxonEngine project, and may only be used, modified, 
and distributed under the terms of the AxonEngine project license, license.txt.  
By continuing to use, modify, or distribute this file you indicate that you have
read the license and understand and accept it fully.
*/

#include "atomicint.h"

#ifndef AX_THREAD_H 
#define AX_THREAD_H

AX_BEGIN_NAMESPACE

#define AX_INFINITE 0xFFFFFFFF

//------------------------------------------------------------------------------
class AX_API SyncObject
{
public:
	virtual ~SyncObject() {}

	virtual bool lock(uint_t timeout = AX_INFINITE) = 0;
	virtual bool unlock() = 0;
};

//------------------------------------------------------------------------------
class AX_API SyncMutex
{
public:
	SyncMutex();
	virtual ~SyncMutex();

	virtual bool lock(uint_t timeout = AX_INFINITE);
	virtual bool unlock();

private:
	mutable void *m_object;
};

//------------------------------------------------------------------------------

#define SCOPED_LOCK ScopedLocker __scopeLocker(m_mutex);

class AX_API ThreadSafe
{
public:
	mutable SyncMutex m_mutex;
};

class ScopedLocker
{
public:
	ScopedLocker(SyncMutex &syncobject) : m_mutex(syncobject) { m_mutex.lock(); }
	~ScopedLocker() { m_mutex.unlock(); }
private:
	SyncMutex &m_mutex;
};

//------------------------------------------------------------------------------

class AX_API SyncEvent
{
public:
	SyncEvent();
	virtual ~SyncEvent();

	virtual bool lock(uint_t timeout = AX_INFINITE);
	virtual bool unlock();

	bool setEvent();
	bool pulseEvent();
	bool resetEvent();

private:
	void *m_object;
};

//------------------------------------------------------------------------------

class INotifyHandler;
class AX_API Thread
{
public:
	enum RunningStatus {
		RS_Exit, RS_Continue
	};

	Thread();
	Thread(ulong_t id);
	virtual ~Thread();

	void startThread();
	void stopThread();
	bool isCurrentThread() const;

	void addAsyncNotify(INotifyHandler *handler, int index);
	void dispatchAsyncNotify();

	virtual RunningStatus doRun() { return RS_Continue; } // work entry

	static Thread *getThreadById(ulong_t id);
	static Thread *getCurrentThread();
	static Thread *getMainThread();
	static bool isInMainThread();

protected:
	static void checkMainThread();

private:
	Handle m_handle;
	SyncEvent *m_exitEvent;
	ulong_t m_id;

	struct AsyncNotify {
		INotifyHandler *handler;
		int index;
	};
	std::list<AsyncNotify> m_asyncNotifyList;
	SyncMutex m_asyncNotifyMutex;

	static Thread *ms_mainThread;
};

AX_END_NAMESPACE

#endif // AX_THREAD_H
