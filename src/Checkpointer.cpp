#include <oscpp/Checkpointer.h>

#ifdef _WIN32

  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>

  namespace oscpp
  {
    inline LARGE_INTEGER *  checkpointerPtr(void * const t)  { return  reinterpret_cast<LARGE_INTEGER * >(t); }
    inline LARGE_INTEGER *  newCheckpointer()                { return new LARGE_INTEGER; }
    inline void             getCheckpoint(void * t)          { QueryPerformanceCounter(checkpointerPtr(t)); }
    inline double           checkpointerSeconds(void * start, void * end)
    {
      LARGE_INTEGER * t0 = checkpointerPtr(start);
      LARGE_INTEGER * t1 = checkpointerPtr(end);
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
    inline struct timeval * checkpointerPtr(void * const t)  { return  reinterpret_cast<struct timeval * >(t); }
    inline struct timeval * newCheckpointer()                { return new struct timeval; }
    inline void             getCheckpoint(void * t)          { gettimeofday(checkpointerPtr(t), NULL); }
    inline double           checkpointerSeconds(void * start, void * end)
    {
      struct timeval * t0 = checkpointerPtr(start);
      struct timeval * t1 = checkpointerPtr(end);
      double ret;
      return static_cast<double>(t1->tv_sec - t0->tv_sec) + static_cast<double>(t1->tv_usec - t0->tv_usec) / 1000000.0;
    }
  }

#endif

namespace oscpp
{
  Checkpointer::Checkpointer(const int maximumNumberOfEvents) : maxTimes(maximumNumberOfEvents)
  {
    times = new void*[maxTimes];
    numTimes = 0;
    for (int i = 0; i < maxTimes; ++i) times[i] = newCheckpointer();
  }
  Checkpointer::~Checkpointer()
  {
    delete [] times;
  }


  void Checkpointer::check()
  {
    getCheckpoint(times[numTimes]);
    if (numTimes < maxTimes) ++numTimes;
  }
  void Checkpointer::reset()
  {
    numTimes = 0;
  }
  double Checkpointer::getElapsedMicroseconds() const
  {
    return getElapsedMicroseconds(0, numTimes - 1);
  }
  double Checkpointer::getElapsedMilliseconds() const
  {
    return getElapsedMilliseconds(0, numTimes - 1);
  }
  double Checkpointer::getElapsedSeconds() const
  {
    return getElapsedSeconds(0, numTimes - 1);
  }
  double Checkpointer::getElapsedMicroseconds(const int checkpointNumber) const
  {
    return getElapsedMicroseconds(checkpointNumber - 1, checkpointNumber);
  }
  double Checkpointer::getElapsedMilliseconds(const int checkpointNumber) const
  {
    return getElapsedMilliseconds(checkpointNumber - 1, checkpointNumber);
  }
  double Checkpointer::getElapsedSeconds(const int checkpointNumber) const
  {
    return getElapsedSeconds(checkpointNumber - 1, checkpointNumber);
  }
  double Checkpointer::getElapsedMicroseconds(const int cpStart, const int cpEnd) const
  {
    return checkpointerSeconds(times[cpStart], times[cpEnd]) * 1000000.0;
  }
  double Checkpointer::getElapsedMilliseconds(const int cpStart, const int cpEnd) const
  {
    return checkpointerSeconds(times[cpStart], times[cpEnd]) * 1000.0;
  }
  double Checkpointer::getElapsedSeconds(const int cpStart, const int cpEnd) const
  {
    return checkpointerSeconds(times[cpStart], times[cpEnd]);
  }
}
