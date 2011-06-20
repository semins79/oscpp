#include <oscpp/Timer.h>

#ifdef _WIN32

  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>

  namespace oscpp
  {
    inline LARGE_INTEGER *  timerPtr(void * const t)  { return  reinterpret_cast<LARGE_INTEGER * >(t); }
    inline LARGE_INTEGER *  newTimer()                { return new LARGE_INTEGER; }
    inline void             getTime(void * t)         { QueryPerformanceCounter(timerPtr(t)); }
    inline double           timerSeconds0(void * start, void * end)
    {
      LARGE_INTEGER * t0 = timerPtr(start);
      LARGE_INTEGER * t1 = timerPtr(end);
      LARGE_INTEGER freq, t;
      QueryPerformanceFrequency(&freq);
      t.QuadPart = t1->QuadPart - t0->QuadPart;
      return static_cast<double>(t.QuadPart) / static_cast<double>(freq.QuadPart);
    }
    inline double           timerSeconds1(void * start, void * end)
    {
      getTime(end);
      LARGE_INTEGER * t0 = timerPtr(start);
      LARGE_INTEGER * t1 = timerPtr(end);
      LARGE_INTEGER freq, t;
      QueryPerformanceFrequency(&freq);
      t.QuadPart = t1->QuadPart - t0->QuadPart;
      return static_cast<double>(t.QuadPart) / static_cast<double>(freq.QuadPart);
    }
  }

#else

  #include <sys/time.h>
  #include <cstddef> // for NULL

  namespace oscpp
  {
    inline struct timeval * timerPtr(void * const t)  { return  reinterpret_cast<struct timeval * >(t); }
    inline struct timeval * newTimer()                { return new struct timeval; }
    inline void             getTime(void * t)         { gettimeofday(timerPtr(t), NULL); }
    inline double           timerSeconds0(void * start, void * end)
    {
      struct timeval * t0 = timerPtr(start);
      struct timeval * t1 = timerPtr(end);
      double ret;
      return static_cast<double>(t1->tv_sec - t0->tv_sec) + static_cast<double>(t1->tv_usec - t0->tv_usec) / 1000000.0;
    }
    inline double           timerSeconds1(void * start, void * end)
    {
      getTime(end);
      struct timeval * t0 = timerPtr(start);
      struct timeval * t1 = timerPtr(end);
      double ret;
      return static_cast<double>(t1->tv_sec - t0->tv_sec) + static_cast<double>(t1->tv_usec - t0->tv_usec) / 1000000.0;
    }
  }

#endif

namespace oscpp
{
  Timer::Timer() : t0(newTimer()), t1(newTimer()), running(false)
  {
  }
  Timer::~Timer()
  {
    delete timerPtr(t0);
    delete timerPtr(t1);
  }

  void Timer::start()
  {
    running = true;
    getTime(t0);
  }
  void Timer::stop()
  {
    running = false;
    getTime(t1);
  }

  double Timer::getElapsedMicroseconds() const
  {
    return getElapsedSeconds() * 1000000.0;
  }
  double Timer::getElapsedMilliseconds() const
  {
    return getElapsedSeconds() * 1000.0;
  }
  double Timer::getElapsedSeconds() const
  {
    if (running) return timerSeconds1(t0, t1);
    return timerSeconds0(t0, t1);
  }
}
