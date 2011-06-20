#include <oscpp/Condition.h>
#include <oscpp/Mutex.h>

#include <pthread.h>

inline static pthread_cond_t & ref(void * t) { return *reinterpret_cast<pthread_cond_t * >(t); }
inline static pthread_cond_t * ptr(void * t) { return  reinterpret_cast<pthread_cond_t * >(t); }

namespace oscpp
{
  Condition::Condition()
  {
    handle = new pthread_cond_t;
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
    ref(handle) = cond;
  }

  Condition::~Condition()
  {
    delete ptr(handle);
  }

  void Condition::signal()
  {
    pthread_cond_signal(ptr(handle));
  }
  void Condition::broadcast()
  {
    pthread_cond_broadcast(ptr(handle));
  }
  void Condition::wait()
  {
    pthread_cond_wait(ptr(handle), reinterpret_cast<pthread_mutex_t * >(mutex.getHandle()));
  }
  void Condition::lockMutex()
  {
    mutex.lock();
  }
  void Condition::unlockMutex()
  {
    mutex.unlock();
  }
}
