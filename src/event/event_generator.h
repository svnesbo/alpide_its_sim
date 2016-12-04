#ifndef EVENT_GENERATOR_H
#define EVENT_GENERATOR_H

#include "event.h"
#include <set>
#include "TRandom3.h"


class EventGenerator {
private:
  std::queue<Event*> mEventQueue;  

  int mBunchCrossingRateNs = 25;

  // mBunchCrossingRateNs x mGapFactor
  int mAverageCrossingRateNs;

  // 1 - (Number of "gaps" among bunches / possible number of bunches)
  double mGapFactor = 1.0;

  // Number of events to keep in memory at a time. 
  int mNumEventsInMemory = 1;

  int mTotalNumberOfEvents = 0;

  int mRandomSeed;

  int mHitMultiplicityAverage = 1000;
  int mHitMultiplicityDeviation = 300;


public:
  EventGenerator();
  void createEvent();
  void createEvents(int n_events);
  const Event& getNextEvent(void);
  void setBunchCrossingRate(int rate_ns);
  void setBunchGapFactor(double factor);
  void setRandomSeed(int seed);
  void initRandomNumGenerator(void);
  void setPath(std::string path);
  void enableWriteToDisk(void);
  void disableWriteToDisk(void);
  void setNumEventsInMem(int n);
  int getEventsInMem(void);
  int getTotalNumberOfEvents(void);
};



#endif
