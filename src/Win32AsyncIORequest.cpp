#ifdef _WIN32

#include <oscpp/Win32AsyncIORequest.h>
#include <strsafe.h>

namespace oscpp
{

  Win32AsyncIORequest::Win32AsyncIORequest(const int pReqType, HANDLE pFileHandle, const bool alreadyFinished)
    : AsyncIORequest(pReqType), numBytes(0), done(alreadyFinished), error(false), fileHandle(pFileHandle)
  {
    event = CreateEvent(NULL, TRUE, FALSE, NULL);
  }
  Win32AsyncIORequest::~Win32AsyncIORequest()
  {
    sync();
    CloseHandle(event);
  }

  bool Win32AsyncIORequest::query()
  {
    if (done || error) return true;

    DWORD byteCount;

    DWORD status = WaitForSingleObject(overlapped.hEvent, 0);
    switch (status)
    {
    case WAIT_TIMEOUT:  // not done
      return false;
      break;
    case WAIT_OBJECT_0: // done
      break;
    }

    BOOL ret = GetOverlappedResult(fileHandle, &overlapped, &byteCount, FALSE);
    if (!ret)
    {
      DWORD lastError = GetLastError();
      if (lastError != ERROR_IO_PENDING)
      {
        raiseError();

        LPVOID lpMsgBuf;
        LPVOID lpDisplayBuf;
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                      FORMAT_MESSAGE_FROM_SYSTEM |
                      FORMAT_MESSAGE_IGNORE_INSERTS,
                      NULL,
                      lastError,
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      (LPTSTR) &lpMsgBuf,
                      0,
                      NULL);
        lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
                                          (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen("GetOverlappedResult") + 40) * sizeof(TCHAR));
        StringCchPrintf((LPTSTR)lpDisplayBuf,
                        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
                        TEXT("%s failed with error %d: %s"),
                        "GetOverlappedResult", lastError, lpMsgBuf);
        printf("%s\n", lpDisplayBuf); fflush(stdout);

        LocalFree(lpMsgBuf);
        LocalFree(lpDisplayBuf);

        return true;
      }
      return false;
    }
    done = true;
    numBytes = static_cast<int>(byteCount);
    return true;
  }
  void Win32AsyncIORequest::sync()
  {
    if (!error && !done)
    {
      DWORD byteCount;
      BOOL ret = GetOverlappedResult(fileHandle, &overlapped, &byteCount, TRUE);
      if (!ret) raiseError();
      numBytes = byteCount;
    }
  }
  bool Win32AsyncIORequest::hasError()
  {
    return error;
  }
  int Win32AsyncIORequest::bytesTransferedCount()
  {
    return numBytes;
  }
}

#endif
