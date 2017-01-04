/**
 * @file   event_generator.h
 * @Author Simon Voigt Nesbo
 * @date   December 22, 2016
 * @brief  A simple event generator for Alpide SystemC simulation model.
 */

#ifndef EVENT_GENERATOR_H
#define EVENT_GENERATOR_H

#include "trigger_event.h"
#include <systemc.h>
#include <queue>
#include <deque>
#include <fstream>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/exponential_distribution.hpp>
#include <boost/random/discrete_distribution.hpp>
#include <cstdint>

using std::int64_t;

//@todo Define this somewhere more appropriate
//#define N_CHIPS 25000

// 108 chips in innermost layer
#define N_CHIPS 108

//@todo Destructor! We have to clean up after ourselves!
//@todo It would be nice if these classes could create the data subdirectory themselves..
class EventGenerator : sc_core::sc_module
{
public: // SystemC signals  
  sc_in<bool> s_strobe_in;
  sc_in_clk s_clk_in;
  sc_event_queue_port E_trigger_event_available;

private:
  std::queue<TriggerEvent*> mEventQueue;

  // This is a pointer to the next trigger event which is "under construction".
  // It is created on rising edge of strobe signal, and completed (and moved to mEventQueue) on
  // the corresponding falling edge of the strobe signal.
  TriggerEvent* mNextTriggerEvent = nullptr;
  
  // New hits will be push at the back, and old (expired) hits popped at the front.
  // We need to be able to iterate over the queue, so a normal std::queue would not work.
  // And deque seems faster than a list for our purpose:
  // http://stackoverflow.com/questions/14574831/stddeque-or-stdlist
  // But that should probably be tested :)
  std::deque<Hit> mHitQueue;

  int mBunchCrossingRateNs;

  int mAverageEventRateNs;

  //@todo Remove. I don't need to know this after all. I am actually generating things real time,
  //      keeping hits in memory for as long as necessary, and moving hits that were active during
  //      an event to mNextTriggerEvent at falling edge of strobe.
  int mStrobeLengthNs;

  // Number of events to keep in memory at a time. 0 = infinite.
  int mNumEventsInMemoryAllowed = 0;

  // Total number of physics and trigger events generated.
  int mPhysicsEventCount = 0;
  int mTriggerEventIdCount = 0;

  // Time of the last physics event that was generated.
  int64_t mLastPhysicsEventTimeNs = 0;

  // Time of the last trigger event that was generated (time of last strobe)
  // Will not be updated if trigger was filtered out.
  int64_t mLastTriggerEventStartTimeNs = 0;
  int64_t mLastTriggerEventEndTimeNs = 0;  

  int mPixelDeadTime;
  int mPixelActiveTime;
  
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

  // Uniform distribution used generating hit coordinates
  boost::random::uniform_int_distribution<int> *mRandHitChipID, *mRandHitChipX, *mRandHitChipY;

  // Choice of discrete distribution (based on discrete list of N_hits vs Probability),
  // or gaussian distribution.
  boost::random::discrete_distribution<> *mRandHitMultiplicityDiscrete;
  boost::random::normal_distribution<double> *mRandHitMultiplicityGauss;

  // Exponential distribution used for time between events
  boost::random::exponential_distribution<double> *mRandEventTime;

  int mHitMultiplicityGaussAverage;
  int mHitMultiplicityGaussDeviation;

  void calculateAverageCrossingRate(void);
  void eventMemoryCountLimiter(void);

public:
  EventGenerator(sc_core::sc_module_name name);
  EventGenerator(sc_core::sc_module_name name,
                 int BC_rate_ns, int avg_event_rate_ns, int strobe_length_ns,
                 int hit_mult_avg, int hit_mult_dev,
                 int pixel_dead_time_ns, int pixel_active_time_ns,
                 int random_seed = 0, bool create_csv_hit_file = false);
  EventGenerator(sc_core::sc_module_name name,
                 int BC_rate_ns, int avg_event_rate_ns, int strobe_length_ns,
                 const char* mult_dist_filename,
                 int pixel_dead_time_ns, int pixel_active_time_ns,
                 int random_seed = 0, bool create_csv_hit_file = false);
  ~EventGenerator();
  void generateNextEvent();
  void generateNextEvents(int n_events);
  const TriggerEvent& getNextTriggerEvent(void) const;
  void setBunchCrossingRate(int rate_ns);
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
  int getPhysicsEventCount(void) const {return mPhysicsEventCount;}
  int getTriggerEventCount(void) const {return mTriggerEventIdCount;}
  void removeOldestEvent(void);
  void physicsEventProcess(void);
  void triggerEventProcess(void);
  
private:
  TriggerEvent* generateNextTriggerEvent(int64_t event_start);
  int64_t generateNextPhysicsEvent(void);
  void readDiscreteDistributionFile(const char* filename, std::vector<double> &dist_vector) const;
  unsigned int getRandomMultiplicity(void);
  void addHitsToTriggerEvent(TriggerEvent& e);
  void removeInactiveHits(void);
};



#endif
