#include <oscpp/Thread.h>
#include <oscpp/Runnable.h>

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#else
  #include <sched.h>
  #include <unistd.h>
#endif

#include <pthread.h>
#include <sched.h>

inline static pthread_t & ref(void * t) { return *reinterpret_cast<pthread_t * >(t); }
inline static pthread_t * ptr(void * t) { return  reinterpret_cast<pthread_t * >(t); }

namespace oscpp
{

  void * Thread::startThread(void * vself)
  {
    Thread * const self = reinterpret_cast<Thread * >(vself);
    self->runner->run();
    self->running = false;
    return NULL;
  }

  Thread::Thread(Runnable * const pRunner) : handle(NULL), running(false), runner(pRunner)
  {
    handle = new pthread_t;
  }
  Thread::~Thread()
  {
    join();
    delete ptr(handle);
  }

  void Thread::yield()
  {
    sched_yield();
  }
  void Thread::start()
  {
    if (running) return;
    running = true;
    pthread_create(ptr(handle), NULL, startThread, this);
  }
  void Thread::run()
  {
    if (running) return;
    running = true;
    runner->run();
    running = false;
  }
  void Thread::join()
  {
    if (running) pthread_join(ref(handle), NULL);
  }
  bool Thread::isRunning()
  {
    return running;
  }
  void Thread::bindToProcessor(const int zeroBasedProcID)
  {
#if _WIN32
    SetProcessAffinityMask(GetCurrentProcess(), 1 << zeroBasedProcID);
#else
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(zeroBasedProcID, &set);
    sched_setaffinity(getpid(), CPU_SETSIZE, &set);
#endif
  }
}
