#ifndef EVENT_GENERATOR_H
#define EVENT_GENERATOR_H

#include "event.h"
#include <queue>
#include "TRandom3.h"


//@todo Destructor! We have to clean up after ourselves!
//@todo It would be nice if these classes could create the data subdirectory themselves..
class EventGenerator {
private:
  std::queue<Event*> mEventQueue;  

  int mBunchCrossingRateNs;

  //@todo For more accuracy, maybe this factor and average crossing rate should be replaced
  //      with an actual filling pattern?
  //@brief Calculated as: mBunchCrossingRateNs x mGapFactor  
  int mAverageCrossingRateNs;

  // 1 - (Number of "gaps" among bunches / possible number of bunches)
  double mGapFactor;

  // Number of events to keep in memory at a time. 0 = infinite.
  int mNumEventsInMemoryAllowed = 0;

  // Total number of events generated.
  int mEventCount = 0;

  // Time of the last event that was generated.
  int mLastEventTimeNs = 0;

  std::string mDataPath = "data";
  bool mWriteEventsToDisk = false;

  int mRandomSeed;

  TRandom mRandHitGen;
  TRandom mRandHitMultiplicityGen;
  TRandom mRandEventTimeGen;

  int mHitMultiplicityAverage;
  int mHitMultiplicityDeviation;

  void calculateAverageCrossingRate(void);
  void eventMemoryCountLimiter(void);

public:
  EventGenerator();
  EventGenerator(int BC_rate_ns, double gap_factor, int hit_mult_avg, int hit_mult_dev, int random_seed = 0);
  void generateNextEvent();
  void generateNextEvents(int n_events);
  const Event& getNextEvent(void) const;
  void setBunchCrossingRate(int rate_ns);
  void setBunchGapFactor(double factor);
  void setRandomSeed(int seed);
  void initRandomNumGenerator(void);
  void setPath(const std::string& path) {mDataPath = path;}
  void enableWriteToDisk(void) {mWriteEventsToDisk = true;}
  void disableWriteToDisk(void) {mWriteEventsToDisk = false;}
  void setNumEventsInMemAllowed(int n);
  int getEventsInMem(void) const {return mEventQueue.size();}
  int getEventCount(void) const {return mEventCount;}
  void removeOldestEvent(void);
};



#endif
