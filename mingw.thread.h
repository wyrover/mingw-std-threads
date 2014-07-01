#ifndef WIN32STDTHREAD_H
#define WIN32STDTHREAD_H

#include <windows.h>
#include <functional>
#include <memory>
#include <chrono>
#include <system_error>

namespace std
{

class thread
{
public:
    class id
    {
        DWORD mId;
        void clear() {mId = 0;}
        friend class thread;
    public:
        id(DWORD aId=0):mId(aId){}
        bool operator==(const id& other) const {return mId == other.mId;}
    };
protected:
    HANDLE mHandle;
    id mThreadId;
public:
    typedef HANDLE native_handle_type;
    id get_id() const noexcept {return mThreadId;}
    native_handle_type native_handle() const {return mHandle;}
    thread(): mHandle(INVALID_HANDLE_VALUE){}
    thread(thread& other)
    :mHandle(other.mHandle), mThreadId(other.mThreadId)
    {
        other.mHandle = INVALID_HANDLE_VALUE;
        other.mThreadId.clear();
    }
    template<class Function, class... Args>
    explicit thread(Function&& f, Args&&... args)
    {
        typedef decltype(std::bind(f, args...)) Call;
        Call* call = new Call(std::bind(f, args...));
        mHandle = CreateThread(NULL, 0, threadfunc<Call>,
            (LPVOID)call, 0, &(mThreadId.mId));
    }
    template <class Call>
    static unsigned long __stdcall threadfunc(void* arg)
    {
        std::unique_ptr<Call> upCall(static_cast<Call*>(arg));
        (*upCall)();
        return (unsigned long)0;
    }
    bool joinable() const {return mHandle != INVALID_HANDLE_VALUE;}
    void join()
    {
        if (get_id() == GetCurrentThreadId())
            throw system_error(EDEADLK, generic_category());
        if (mHandle == INVALID_HANDLE_VALUE)
            throw system_error(ESRCH, generic_category());
        if (!joinable())
            throw system_error(EINVAL, generic_category());
        WaitForSingleObject(mHandle, INFINITE);
        CloseHandle(mHandle);
        mHandle = INVALID_HANDLE_VALUE;
        mThreadId.clear();
    }

    ~thread()
    {
        if (joinable())
            std::terminate();
    }
    thread& operator=(const thread&) = delete;
    thread& operator=(thread&& other) noexcept
    {
        if (joinable())
          std::terminate();
        swap(std::forward<thread>(other));
        return *this;
    }
    void swap(thread&& other) noexcept
    {
        std::swap(mHandle, other.mHandle);
        std::swap(mThreadId.mId, other.mThreadId.mId);
    }
    static unsigned int hardware_concurrency() noexcept {return 1;}
    void detach()
    {
        if (!joinable())
            throw system_error();
        mHandle = INVALID_HANDLE_VALUE;
        mThreadId.clear();
    }
};
namespace this_thread
{
    thread::id get_id() {return thread::id(GetCurrentThreadId());}
    void yield() {Sleep(0);}
    template< class Rep, class Period >
    void sleep_for( const std::chrono::duration<Rep,Period>& sleep_duration)
    {
        Sleep(chrono::duration_cast<chrono::milliseconds>(sleep_duration).count());
    }
    template <class Clock, class Duration>
    void sleep_until(const std::chrono::time_point<Clock,Duration>& sleep_time)
    {
        sleep_for(sleep_time-Clock::now());
    }
}

}
#endif // WIN32STDTHREAD_H