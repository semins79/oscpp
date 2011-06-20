#include <oscpp/AsyncIORequest.h>

namespace oscpp
{
  AsyncIORequest::AsyncIORequest(const int pReqType) : reqType(pReqType)
  {
  }
  AsyncIORequest::AsyncIORequest(const AsyncIORequest & rhs) : reqType(rhs.reqType)
  {
  }
  AsyncIORequest & AsyncIORequest::operator = (const AsyncIORequest & rhs)
  {
    reqType = rhs.reqType;
    return * this;
  }

  AsyncIORequest::~AsyncIORequest()
  {
  }
}
