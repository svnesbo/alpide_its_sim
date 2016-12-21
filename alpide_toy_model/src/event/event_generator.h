#ifndef EVENT_GENERATOR_H
#define EVENT_GENERATOR_H

#include "event.h"
#include <queue>
#include <fstream>
// #include <boost/random/uniform_int_distribution.hpp>
// #include <boost/random/normal_distribution.hpp>
// #include <boost/random/exponential_distribution.hpp>
//#include <boost/generator_iterator.hpp>
#include <boost/random.hpp>

//@todo Define this somewhere more appropriate
#define N_CHIPS 25000

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

  // Minimum time between two triggers/events. Triggers/events that come sooner than this will
  // be filtered out (but their hits will still be stored).
  int mTriggerFilterTimeNs;
  bool mTriggerFilteringEnabled = false;

  std::string mDataPath = "data";
  bool mWriteEventsToDisk = false;

  bool mWriteRandomDataToFile = true;
  std::ofstream mRandDataFile;
  
  int mRandomSeed;

  boost::random::mt19937 mRandHitGen;
  boost::random::mt19937 mRandHitMultiplicityGen;
  boost::random::mt19937 mRandEventTimeGen;

  // Use pointers? Makes initialization easier?
  boost::random::uniform_int_distribution<int> *mRandHitChipID, *mRandHitChipX, *mRandHitChipY;
  boost::random::normal_distribution<double> *mRandHitMultiplicity;
  boost::random::exponential_distribution<double> *mRandEventTime;

  int mHitMultiplicityAverage;
  int mHitMultiplicityDeviation;

  void calculateAverageCrossingRate(void);
  void eventMemoryCountLimiter(void);

public:
  EventGenerator();
  //EventGenerator(int BC_rate_ns, double gap_factor, int hit_mult_avg, int hit_mult_dev, int random_seed = 0);
  EventGenerator(int BC_rate_ns, int avg_trigger_rate_ns, int hit_mult_avg, int hit_mult_dev, int random_seed = 0);
  ~EventGenerator();
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
  void setTriggerFilterTime(int filter_time) {mTriggerFilterTimeNs = filter_time;}
  void enableTriggerFiltering(void) {mTriggerFilteringEnabled = true;}
  void disableTriggerFiltering(void) {mTriggerFilteringEnabled = false;}
  int getTriggerFilterTime(void) const {return mTriggerFilterTimeNs;}
  int getEventsInMem(void) const {return mEventQueue.size();}
  int getEventCount(void) const {return mEventCount;}
  void removeOldestEvent(void);
};



#endif
