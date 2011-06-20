#include <oscpp/AsyncFileReader.h>
#include <oscpp/AsyncIORequest.h>
#include <oscpp/Timer.h>

#include <cuda_runtime_api.h>

#include <algorithm>
#include <vector>

#include <cstdlib>
#include <cstdio>

const int MAX_READ_SIZE = 32 * 1024 * 1024;

int queuedReads(int argc, char ** argv)
{
  oscpp::AsyncFileReader * reader = new oscpp::AsyncFileReader;
  oscpp::AsyncIORequest * openReq, * closeReq;
  std::vector<oscpp::AsyncIORequest * > readReqs;
  void * buf;
  int numBytes = atoi(argv[3]);

  printf("opening '%s'... ", argv[1]); fflush(stdout);
  openReq = reader->open(argv[1]);
  openReq->sync();
  if (openReq->hasError())
  {
    fprintf(stderr, "Couldn't open '%s'.\n", argv[1]);
    delete reader;
    delete openReq;
    return 1;
  }
  delete openReq;
  printf("done.\n"); fflush(stdout);

  printf("asynchronously reading %d bytes... ", numBytes); fflush(stdout);
  cudaMallocHost(&buf, numBytes);

  int dataLeft = numBytes;
  int soFar = 0;
  while (dataLeft > 0)
  {
    const int toRead = std::min(numBytes, MAX_READ_SIZE);
    readReqs.push_back(reader->read((char * )buf + soFar, toRead));
    if (readReqs.back()->hasError())
    {
      fprintf(stderr, "Error issuing read request %u.\n", readReqs.size());
      delete reader;
      for (unsigned int i = 0; i < readReqs.size(); ++i) delete readReqs[i];
      return 1;
    }
    soFar += toRead;
    dataLeft -= toRead;
  }

  printf("done with request.\n"); fflush(stdout);

  printf("querying to see if request is done ten times"); fflush(stdout);
  bool allDone = false;
  for (int i = 0; i < 10; ++i)
  {
    unsigned int numDone = 0;
    for (unsigned int j = 0; j < readReqs.size(); ++j)
    {
      if (readReqs[j]->query()) ++numDone;
    }
    if (numDone == readReqs.size())
    {
      printf(" - read request finished after querying %d times.\n", i + 1); fflush(stdout);
      allDone = true;
      break;
    }
    else
    {
      printf("."); fflush(stdout);
    }
  }
  if (!allDone)
  {
    printf("read request wasn't finished, syncing... "); fflush(stdout);
    for (unsigned int j = 0; j < readReqs.size(); ++j) readReqs[j]->sync();
    printf("done.\n"); fflush(stdout);
  }
  int totalBytes = 0;
  for (unsigned int j = 0; j < readReqs.size(); ++j)
  {
    if (readReqs[j]->hasError())
    {
      fprintf(stderr, "Error occurred during reading.\n"); fflush(stderr);
      delete reader;
      for (unsigned int k = j; k < readReqs.size(); ++k) delete readReqs[k];
      return 1;
    }
    totalBytes += readReqs[j]->bytesTransferedCount();
    delete readReqs[j];
  }
  printf("read %d bytes.\n", totalBytes); fflush(stdout);

  printf("closing file... "); fflush(stdout);
  closeReq = reader->close();
  closeReq->sync();
  if (closeReq->hasError())
  {
    fprintf(stderr, "Error closing.\n");
    delete reader;
    delete closeReq;
    return 1;
  }
  delete closeReq;
  delete reader;
  printf("done.\n"); fflush(stdout);

  return 0;
}
int waitReads(int argc, char ** argv)
{
  oscpp::AsyncFileReader * reader = new oscpp::AsyncFileReader;
  oscpp::AsyncIORequest * openReq, * readReq, * closeReq;
  void * buf;
  int numBytes = atoi(argv[3]);

  printf("opening '%s'... ", argv[2]); fflush(stdout);
  openReq = reader->open(argv[2]);
  openReq->sync();
  if (openReq->hasError())
  {
    fprintf(stderr, "Couldn't open '%s'.\n", argv[2]);
    delete reader;
    delete openReq;
    return 1;
  }
  delete openReq;
  printf("done.\n"); fflush(stdout);

  cudaMallocHost(&buf, numBytes);

  int dataLeft = numBytes;
  int soFar = 0;
  while (dataLeft > 0)
  {
    int toRead = std::min(numBytes, MAX_READ_SIZE);
    printf("asynchronously reading %d bytes... ", toRead); fflush(stdout);
    readReq = reader->read((char * )buf + soFar, toRead);
    if (readReq->hasError())
    {
      fprintf(stderr, "Error issuing read request.\n");
      delete reader;
      delete readReq;
      return 1;
    }
    printf("done issuing read request.\n"); fflush(stdout);
    printf("querying to see if request is done ten times"); fflush(stdout);
    for (int i = 0; i < 10; ++i)
    {
      if (readReq->query())
      {
        printf(" - read request finished after querying %d times.\n", i + 1); fflush(stdout);
        break;
      }
      else
      {
        printf("."); fflush(stdout);
      }
    }
    if (!readReq->query()) readReq->sync();
    if (readReq->hasError())
    {
      fprintf(stderr, "Error occurred during reading.\n"); fflush(stderr);
      delete reader;
      delete readReq;
      return 1;
    }
    toRead = readReq->bytesTransferedCount();
    delete readReq;
    printf("read %d bytes.\n", toRead); fflush(stdout);
    soFar += toRead;
    dataLeft -= toRead;
  }
  printf("done with reading.\n"); fflush(stdout);


  printf("closing file... "); fflush(stdout);
  closeReq = reader->close();
  closeReq->sync();
  if (closeReq->hasError())
  {
    fprintf(stderr, "Error closing.\n");
    delete reader;
    delete closeReq;
    return 1;
  }
  delete closeReq;
  delete reader;
  printf("done.\n"); fflush(stdout);

  return 0;
}

int main(int argc, char ** argv)
{
  if (argc != 4)
  {
    fprintf(stderr, "Usage: %s input_file_1 input_file_2 num_bytes_to_read\n", *argv);
    return 1;
  }

  oscpp::Timer t1, t2;

  printf("reading once to compensate for cache behavior... "); fflush(stdout);
  waitReads(argc, argv);
  printf("done.\n"); fflush(stdout);

  t2.start();
  if (waitReads  (argc, argv) != 0) return 1;
  t2.stop();
  t1.start();
  if (queuedReads(argc, argv) != 0) return 1;
  t1.stop();

  printf("Total time for queued reads:  %.3f ms.\n", t1.getElapsedMilliseconds());
  printf("Total time for wait reads:    %.3f ms.\n", t2.getElapsedMilliseconds());

  return 0;
}
