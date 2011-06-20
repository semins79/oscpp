#ifndef __CHECKPOINTER_H__
#define __CHECKPOINTER_H__

namespace oscpp
{
  /**
   * The timer class gives one access to 10-microsecond accurate timing with
   * multiple checkpoints along the way. This class is more efficient and
   * appropriate for a multi-stage simulation where many individual stages
   * must be timed.
   */
  class Checkpointer
  {
    protected:
      /// The maximum number of checkpoints we can capture.
      int maxTimes;
      /// The number of current checkpoints we've captured.
      int numTimes;
      /// The native handle to the checkpoints.
      void ** times;
    public:
      /// The default constructor
      ///
      /// @param maximumNumberOfTimes The maximum number of events to
      /// checkpoint.
      Checkpointer(const int maximumNumberOfEvents);
      /// Destructor.
      ~Checkpointer();

      /// Whether or not we've started the clock.
      inline bool isRunning() const { return numTimes > 0; }

      /// How many checkpoints we've actually captured.
      inline int numCheckpointsCaptured() const { return numTimes; }

      /// Capture a checkpoint. Overwrites the last checkpoint if we have
      /// already filled up our storage.
      void check();

      /// Clear out all the current checkpoints.
      void reset();

      /**
       * @return The elapsed microseconds between the first checkpoint and
       *         the last checkpoint. Note that the first checkpoint is zero, but
       *         there is no timing information before.
       */
      double getElapsedMicroseconds() const;
      /**
       * @return The elapsed milliseconds between the first checkpoint and
       *         the last checkpoint. Note that the first checkpoint is zero, but
       *         there is no timing information before.
       */
      double getElapsedMilliseconds() const;
      /**
       * @return The elapsed seconds between the first checkpoint and
       *         the last checkpoint. Note that the first checkpoint is zero, but
       *         there is no timing information before.
       */
      double getElapsedSeconds() const;

      /**
       * @param The checkpoint number. Should be in the range [1,maxTimes - 1].
       *
       * @return The elapsed microseconds between the previous checkpoint and
       *         checkpointNumber. Note that the first checkpoint is zero, but
       *         there is no timing information before.
       */
      double getElapsedMicroseconds(const int checkpointNumber) const;
      /**
       * @param The checkpoint number. Should be in the range [1,maxTimes - 1].
       *
       * @return The elapsed milliseconds between the previous checkpoint and
       *         checkpointNumber. Note that the first checkpoint is zero, but
       *         there is no timing information before.
       */
      double getElapsedMilliseconds(const int checkpointNumber) const;
      /**
       * @param The checkpoint number. Should be in the range [1,maxTimes - 1].
       *
       * @return The elapsed seconds between the previous checkpoint and
       *         checkpointNumber. Note that the first checkpoint is zero, but
       *         there is no timing information before.
       */
      double getElapsedSeconds(const int checkpointNumber) const;

      /**
       * @param cpStart The starting checkpoint.
       * @param cpEnd   The ending checkpoint.
       *
       * @return The elapsed microseconds between the start checkpoint and
       *         the ending checkpoint.
       */
      double getElapsedMicroseconds(const int cpStart, const int cpEnd) const;
      /**
       * @param cpStart The starting checkpoint.
       * @param cpEnd   The ending checkpoint.
       *
       * @return The elapsed milliseconds between the start checkpoint and
       *         the ending checkpoint.
       */
      double getElapsedMilliseconds(const int cpStart, const int cpEnd) const;
      /**
       * @param cpStart The starting checkpoint.
       * @param cpEnd   The ending checkpoint.
       *
       * @return The elapsed seconds between the start checkpoint and
       *         the ending checkpoint.
       */
      double getElapsedSeconds(const int cpStart, const int cpEnd) const;
  };
}

#endif
