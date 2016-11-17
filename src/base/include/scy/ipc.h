///
//
// LibSourcey
// Copyright (c) 2005, Sourcey <http://sourcey.com>
//
// SPDX-License-Identifier:	LGPL-2.1+
//
/// @addtogroup base
/// @{


#ifndef SCY_IPC_H
#define SCY_IPC_H


#include "scy/mutex.h"
#include "scy/synccontext.h"

#include <string>
#include <deque>


namespace scy {


/// Classes for inter-process communication
namespace ipc {


/// Default action type for executing synchronized callbacks.
struct Action
{
    typedef std::function<void(const Action&)> callback_t;
    callback_t target;
    void* arg;
    std::string data;

    Action(callback_t target, void* arg = nullptr, const std::string& data = "") :
        target(target), arg(arg), data(data) {}
};


/// IPC queue is for safely passing templated
/// actions between threads and processes.
template<typename TAction = ipc::Action>
class Queue
{
public:
    Queue()
    {
    }

    virtual ~Queue()
    {
    }

    virtual void push(TAction* action)
    {
        {
            Mutex::ScopedLock lock(_mutex);
            _actions.push_back(action);
        }
        post();
    }

    virtual TAction* pop()
    {
        if (_actions.empty())
            return nullptr;
        Mutex::ScopedLock lock(_mutex);
        TAction* next = _actions.front();
        _actions.pop_front();
        return next;
    }

    virtual void runSync()
    {
        TAction* next = nullptr;
        while ((next = pop())) {
            next->target(*next);
            delete next;
        }
    }

    virtual void close()
    {
    }

    virtual void post()
    {
    }

    void waitForSync()
    {
        // TODO: Impose a time limit
        while(true) {
            {
                Mutex::ScopedLock lock(_mutex);
                if (_actions.empty())
                    return;
            }
            DebugL << "Wait for sync" << std::endl;
            scy::sleep(10);
        }
    }

protected:
    mutable Mutex _mutex;
    std::deque<TAction*> _actions;
};


/// IPC synchronization queue is for passing templated
/// actions between threads and the event loop we are
/// synchronizing with.
template<typename TAction = ipc::Action>
class SyncQueue: public Queue<TAction>
{
public:
    SyncQueue(uv::Loop* loop = uv::defaultLoop()) :
        _sync(loop, std::bind(&Queue<TAction>::runSync, this))
    {
    }

    virtual ~SyncQueue()
    {
    }

    virtual void close()
    {
        _sync.close();
    }

    virtual void post()
    {
        _sync.post();
    }

    virtual SyncContext& sync()
    {
        return _sync;
    }

protected:
    SyncContext _sync;
};


typedef ipc::Queue<ipc::Action> ActionQueue;
typedef ipc::SyncQueue<ipc::Action> ActionSyncQueue;


} // namespace ipc
} // namespace scy


#endif

/// @\}
