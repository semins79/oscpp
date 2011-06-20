#include <oscpp/AsyncFileReader.h>
#include <oscpp/AsyncIORequest.h>
#include <oscpp/Closure.h>
#include <oscpp/Condition.h>
#include <oscpp/GenericAsyncIORequest.h>
#include <oscpp/Runnable.h>
#include <oscpp/Thread.h>

#include <list>
#include <map>
#include <string>

#include <cerrno>

#ifdef _WIN32

  #define WIN32_LEAN_AND_MEAN

  #include <oscpp/Win32AsyncIORequest.h>
  #include <windows.h>
  #include <strsafe.h>

  inline static HANDLE & ref(void * t) { return *reinterpret_cast<HANDLE * >(t); }
  inline static HANDLE * ptr(void * t) { return  reinterpret_cast<HANDLE * >(t); }

  inline HANDLE * asyncFileReaderNewHandle() { return new HANDLE; }
  inline void asyncFileReaderDeleteHandle(HANDLE * handle) { delete handle; }

  inline static oscpp::AsyncIORequest * asyncIORequestOpen(HANDLE & handle, bool & opened, const std::string & fileName)
  {
    handle = CreateFile(fileName.c_str(),
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                        NULL);
    opened = (handle != INVALID_HANDLE_VALUE);
    oscpp::Win32AsyncIORequest * ret = new oscpp::Win32AsyncIORequest(oscpp::AsyncIORequest::REQUEST_TYPE_OPEN, handle, true);
    if      (!opened)         ret->raiseError();
    return ret;
  }
  inline static oscpp::AsyncIORequest * asyncIORequestClose(HANDLE & handle, bool & opened)
  {
    if (opened)
    {
      CloseHandle(handle);
      opened = false;
    }
    return new oscpp::Win32AsyncIORequest(oscpp::Win32AsyncIORequest::REQUEST_TYPE_CLOSE, handle, true);
  }
  inline static oscpp::AsyncIORequest * asyncIORequestRead(HANDLE & handle, bool & opened, unsigned long long & offset, void * const buffer, const size_t size)
  {
    if (!opened) return false;

    oscpp::Win32AsyncIORequest * ret = new oscpp::Win32AsyncIORequest(oscpp::AsyncIORequest::REQUEST_TYPE_READ, handle, false);

    OVERLAPPED & overlapped = ret->getOverlappedStructure();

    overlapped.Internal     = 0;
    overlapped.InternalHigh = 0;
    overlapped.Offset       = offset & 0xFFFFFFFF;
    overlapped.OffsetHigh   = offset >> 32;
    overlapped.hEvent       = ret->getEvent();

    offset += size;

    BOOL val = ReadFile(handle, buffer, size, NULL, &overlapped);

    if (!val)
    {
      DWORD lastError = GetLastError();
      if (lastError != ERROR_IO_PENDING)
      {
        ret->raiseError();


        LPVOID lpMsgBuf;
        LPVOID lpDisplayBuf;
        DWORD dw = GetLastError();

        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                      FORMAT_MESSAGE_FROM_SYSTEM |
                      FORMAT_MESSAGE_IGNORE_INSERTS,
                      NULL,
                      dw,
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      (LPTSTR) &lpMsgBuf,
                      0, NULL );

        // Display the error message and exit the process

        lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
                                          (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen("ReadFile") + 40) * sizeof(TCHAR));
        StringCchPrintf((LPTSTR)lpDisplayBuf,
                        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
                        TEXT("%s failed with error %d: %s"),
                        "ReadFile", dw, lpMsgBuf);
        printf("Error: %s - %d.\n", lpDisplayBuf, lastError);

        LocalFree(lpMsgBuf);
        LocalFree(lpDisplayBuf);

      }
    }
    else // file read was synchronous, probably cached.
    {
      // printf("warning, async file read was executed synchronously.\n");
      // fflush(stdout);
    }

    return ret;
  }

#else // this would be the linux case.

  #include <oscpp/GenericAsyncIORequest.h>

  class GenericAsyncFileReader : public oscpp::Runnable
  {
    protected:
      FILE * fp;
      std::list<oscpp::GenericAsyncIORequest * > pending;
      oscpp::Condition cond;
      volatile bool waiting;

      inline oscpp::AsyncIORequest * submitRequest(oscpp::GenericAsyncIORequest * const req)
      {
        cond.lockMutex();
        pending.push_back(req);
        if (waiting) cond.signal();
        cond.unlockMutex();
        return req;
      }
      inline void handleRequest(oscpp::GenericAsyncIORequest * const req)
      {
        switch (req->getRequestType())
        {
        case oscpp::AsyncIORequest::REQUEST_TYPE_OPEN:
          if (fp != NULL) fclose(fp);
          fp = fopen(reinterpret_cast<char * >(req->getBuffer()), "rb");
          if (fp == NULL) req->raiseError();
          break;
        case oscpp::AsyncIORequest::REQUEST_TYPE_CLOSE:
          if (fp == NULL) req->raiseError();
          else            fclose(fp);
          fp = NULL;
          break;
        case oscpp::AsyncIORequest::REQUEST_TYPE_READ:
          if (fp == NULL)
          {
            req->raiseError();
          }
          else if (fread(req->getBuffer(), req->bytesTransferedCount(), 1, fp) != 1)
          {
            req->raiseError();
          }
          break;
        case oscpp::AsyncIORequest::REQUEST_TYPE_WRITE:
          req->raiseError();
          break;
        }
        req->setDone();
      }
    public:
      inline GenericAsyncFileReader()
      {
        waiting = false;
        fp = NULL;
      }
      virtual ~GenericAsyncFileReader()
      {
      }

      virtual void run()
      {
        while (true)
        {
          cond.lockMutex();
          waiting = true;
          if (pending.empty())
          {
            cond.wait();
          }
          waiting = false;
          std::list<oscpp::GenericAsyncIORequest * > reqs = pending;
          pending.clear();
          cond.unlockMutex();
          while (!reqs.empty())
          {
            if (reqs.front() == NULL) return;
            handleRequest(reqs.front());
            reqs.pop_front();
          }
        }
      }
      inline void finish()
      {
        submitRequest(NULL);
      }
      inline oscpp::AsyncIORequest * open (const std::string & fileName)
      {
        return submitRequest(new oscpp::GenericAsyncIORequest(oscpp::AsyncIORequest::REQUEST_TYPE_OPEN, const_cast<char * >(fileName.c_str()), fileName.size()));
      }
      inline oscpp::AsyncIORequest * close()
      {
        return submitRequest(new oscpp::GenericAsyncIORequest(oscpp::AsyncIORequest::REQUEST_TYPE_CLOSE, NULL, 0));
      }
      inline oscpp::AsyncIORequest * read(void * const buffer, const size_t size)
      {
        return submitRequest(new oscpp::GenericAsyncIORequest(oscpp::AsyncIORequest::REQUEST_TYPE_READ, buffer, size));
      }
  };

  typedef struct _GenericAsyncFileData
  {
    oscpp::Thread * thread;
    GenericAsyncFileReader * runnable;
  } GenericAsyncFileData;

  inline static GenericAsyncFileData & ref(void * t) { return *reinterpret_cast<GenericAsyncFileData * >(t); }
  inline static GenericAsyncFileData * ptr(void * t) { return  reinterpret_cast<GenericAsyncFileData * >(t); }

  inline GenericAsyncFileData * asyncFileReaderNewHandle()
  {
    GenericAsyncFileData * data = new GenericAsyncFileData;
    data->runnable = new GenericAsyncFileReader;
    data->thread = new oscpp::Thread(data->runnable);
    data->thread->start();
    return data;
  }
  inline void asyncFileReaderDeleteHandle(GenericAsyncFileData * data)
  {
    data->runnable->finish();
    data->thread->join();
    delete data->runnable;
    delete data->thread;
    delete data;
  }

  inline static oscpp::AsyncIORequest * asyncIORequestOpen(GenericAsyncFileData & data, bool & opened, const std::string & fileName)
  {
    return data.runnable->open(fileName);
  }
  inline static oscpp::AsyncIORequest * asyncIORequestClose(GenericAsyncFileData & data, bool & opened)
  {
    return data.runnable->close();
  }
  inline static oscpp::AsyncIORequest * asyncIORequestRead(GenericAsyncFileData & data, bool & opened, unsigned long long & offset, void * const buffer, const size_t size)
  {
    return data.runnable->read(buffer, size);
  }

#endif

namespace oscpp
{
  AsyncFileReader::AsyncFileReader()
  {
    offset = 0;
    opened = false;
    handle = asyncFileReaderNewHandle();
  }
  AsyncFileReader::~AsyncFileReader()
  {
    if (opened)
    {
      AsyncIORequest * req = close();
      delete req;
    }
    asyncFileReaderDeleteHandle(ptr(handle));
  }

  AsyncIORequest * AsyncFileReader::open(const std::string & fileName)
  {
    if (opened)
    {
      AsyncIORequest * req = close();
      delete req;
    }
    return asyncIORequestOpen(ref(handle), opened, fileName);
  }
  AsyncIORequest * AsyncFileReader::close()
  {
    return asyncIORequestClose(ref(handle), opened);
  }
  AsyncIORequest * AsyncFileReader::read(void * const buffer, const size_t size)
  {
    return asyncIORequestRead(ref(handle), opened, offset, buffer, size);
  }
}
