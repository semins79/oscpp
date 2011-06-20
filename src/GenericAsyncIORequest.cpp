#ifndef _WIN32

#include <oscpp/GenericAsyncIORequest.h>
#include <cstring>
#include <cstdio>
#include <sched.h>

namespace oscpp
{
  GenericAsyncIORequest::GenericAsyncIORequest(const int pReqType, void * const buf, const size_t numBytes)
    : AsyncIORequest(pReqType), error(false), done(false), buffer(buf), size(numBytes)
  {
    if (reqType == REQUEST_TYPE_OPEN)
    {
      size = numBytes + 1;
      buffer = new char[numBytes + 1];
      memcpy(buffer, buf, numBytes + 1);
    }
  }

  GenericAsyncIORequest::~GenericAsyncIORequest()
  {
    if (reqType == REQUEST_TYPE_OPEN && buffer != NULL) delete [] reinterpret_cast<char * >(buffer);
  }

  bool GenericAsyncIORequest::query()
  {
    return done || error;
  }
  void GenericAsyncIORequest::sync()
  {
    while (!error && !done) sched_yield();
  }
  bool GenericAsyncIORequest::hasError()
  {
    return error;
  }
  int  GenericAsyncIORequest::bytesTransferedCount()
  {
    return static_cast<int>(size);
  }
}

#endif
