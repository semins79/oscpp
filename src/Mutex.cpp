#include <oscpp/Mutex.h>

#include <pthread.h>

inline static pthread_mutex_t & ref(void * t) { return *reinterpret_cast<pthread_mutex_t * >(t); }
inline static pthread_mutex_t * ptr(void * t) { return  reinterpret_cast<pthread_mutex_t * >(t); }

namespace oscpp
{

  Mutex::Mutex()
  {
    handle = new pthread_mutex_t;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    ref(handle) = mutex;
  }
  Mutex::~Mutex()
  {
    pthread_mutex_destroy(ptr(handle));
  }

  bool Mutex::tryLock()
  {
    return pthread_mutex_trylock(ptr(handle)) == 0;
  }
  void Mutex::lock()
  {
    pthread_mutex_lock(ptr(handle));
  }
  void Mutex::unlock()
  {
    pthread_mutex_unlock(ptr(handle));
  }
  void * Mutex::getHandle()
  {
    return handle;
  }
}
