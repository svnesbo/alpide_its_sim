/**
 * @file   event_generator.h
 * @author Simon Voigt Nesbo
 * @date   December 22, 2016
 * @brief  A simple event generator for Alpide SystemC simulation model.
 */

#ifndef EVENT_GENERATOR_H
#define EVENT_GENERATOR_H

#include "trigger_event.h"
#include <systemc.h>
#include <QSettings>
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

// 108 chips in innermost layer
#define N_CHIPS 108

///@brief   A simple event generator for Alpide SystemC simulation model.
///@details Physics events are generated at a rate that has an exponential distribution,
///         with Lambda = 1 / average rate.
///         The number of hits generated (hit multiplicity) per event can be based on a gaussian distribution
///         or a user-defined discrete distribution. The ROOT macro export_multiplicity_data.cxx found under
///         process/Multiplicity_distribution can be used to generate a discrete distribution based on
///         real multiplicity data from ALICE.
///
///         The hits will currently be disributed randomly (with a flat/uniform distribution) among the
///         different chips and over a chip's x/y coordinates.
///         For each hit a fixed 2x2 pixel cluster is generated on the chip (this might be replaced with a
///         more advanced random distribution in the future).
///
///@todo    Get rid off all the constructors. Lets only have one, and configure certain things (like what
///         distribution for multiplicity to use) with functions. There is so much copy-paste between the
///         3 constructors right now, which is hard to keep up to date everywhere.
///@todo    Destructor! We have to clean up after ourselves!
///@todo    It would be nice if these classes could create the data subdirectory themselves..
///@todo    Get rid off all the constructors. Lets only have one, and configure certain things (like what
///         distribution for multiplicity to use) with functions. There is so much copy-paste between the
///         3 constructors right now, which is hard to keep up to date everywhere.
class EventGenerator : sc_core::sc_module
{
public: // SystemC signals  
  sc_in<bool> s_strobe_in;
  sc_in_clk s_clk_in;
  sc_event_queue_port E_trigger_event_available;

  /// Active for one clock pulse every time we have a "physics event".
  /// Not really used for anything, just to indicate physics events in waveforms
  sc_out<bool> s_physics_event_out;

private:
  /// This is the trigger event queue (ie. the hits that
  /// occur between a strobe, which are fed to the Alpide chips).
  /// Each Alpide chip has its own queue (corresponding to an index in the vector).
  std::vector<std::queue<TriggerEvent*> > mEventQueue;
  
  /// This is a pointer to the next trigger event which is "under construction".
  ///  It is created on rising edge of strobe signal, and completed (and moved to mEventQueue) on
  ///  the corresponding falling edge of the strobe signal.
  ///@todo Remove?!
  //TriggerEvent* mNextTriggerEvent = nullptr;
  
  /// New hits will be push at the back, and old (expired) hits popped at the front.
  /// We need to be able to iterate over the queue, so a normal std::queue would not work.
  /// And deque seems faster than a list for our purpose:
  /// http://stackoverflow.com/questions/14574831/stddeque-or-stdlist
  /// But that should probably be tested :)
  /// Each Alpide chip has its own queue (corresponding to an index in the vector).  
  std::vector<std::deque<Hit> > mHitQueue;

  int mNumChips;

  int mBunchCrossingRateNs;

  int mAverageEventRateNs;

  /// Number of events to keep in memory at a time. 0 = infinite.
  int mNumEventsInMemoryAllowed = 0;

  /// Total number of physics and trigger events generated.
  int mPhysicsEventCount = 0;
  int mTriggerEventIdCount = 0;

  /// Time of the last physics event that was generated.
  int64_t mLastPhysicsEventTimeNs = 0;

  /// Time of the last trigger event that was generated (time of last strobe)
  /// Will not be updated if trigger was filtered out.
  int64_t mLastTriggerEventStartTimeNs = 0;
  int64_t mLastTriggerEventEndTimeNs = 0;

  /// Start time of next trigger event (start time recorded on STROBE rising edge).
  /// Event actually created and hits assigned to it on STROBE falling edge.
  int64_t mNextTriggerEventStartTimeNs = 0;

  /// Used by getNextTriggerEvent() so it doesn't have to start iterating from the
  /// beginning of the event queue vector each time it is called.
  /// Also used by removeOldestEvent().
  int mNextTriggerEventChipId = 0;

  int mPixelDeadTime;
  int mPixelActiveTime;
  
  /// Minimum time between two triggers/events. Triggers/events that come sooner than this will
  /// be filtered out (but their hits will still be stored).
  int mTriggerFilterTimeNs;
  bool mTriggerFilteringEnabled = false;

  std::string mDataPath = "data";
  bool mWriteEventsToDisk = false;

  bool mCreateCSVFile = true;
  std::ofstream mRandDataFile;
  
  int mRandomSeed;

  boost::random::mt19937 mRandHitGen;
  boost::random::mt19937 mRandHitMultiplicityGen;
  boost::random::mt19937 mRandEventTimeGen;

  /// Uniform distribution used generating hit coordinates
  boost::random::uniform_int_distribution<int> *mRandHitChipID, *mRandHitChipX, *mRandHitChipY;

  /// Choice of discrete distribution (based on discrete list of N_hits vs Probability),
  /// or gaussian distribution.
  boost::random::discrete_distribution<> *mRandHitMultiplicityDiscrete;
  boost::random::normal_distribution<double> *mRandHitMultiplicityGauss;

  /// Exponential distribution used for time between events
  boost::random::exponential_distribution<double> *mRandEventTime;

  int mHitMultiplicityGaussAverage;
  int mHitMultiplicityGaussDeviation;

  void calculateAverageCrossingRate(void);
  void eventMemoryCountLimiter(void);

public:
//  EventGenerator(sc_core::sc_module_name name);
  EventGenerator(sc_core::sc_module_name name,
                 const QSettings* settings);
/*                 int BC_rate_ns, int avg_event_rate_ns, int strobe_length_ns,
                 int hit_mult_avg, int hit_mult_dev,
                 int pixel_dead_time_ns, int pixel_active_time_ns,
                 int random_seed = 0, bool create_csv_hit_file = false);
  EventGenerator(sc_core::sc_module_name name,
                 int BC_rate_ns, int avg_event_rate_ns, int strobe_length_ns,
                 const char* mult_dist_filename,
                 int pixel_dead_time_ns, int pixel_active_time_ns,
                 int random_seed = 0, bool create_csv_hit_file = false);*/
  ~EventGenerator();
  void generateNextEvent();
  void generateNextEvents(int n_events);
  const TriggerEvent& getNextTriggerEvent(void);
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
  TriggerEvent* generateNextTriggerEvent(int64_t event_start, int64_t event_end, int chip_id);
  int64_t generateNextPhysicsEvent(void);
  void readDiscreteDistributionFile(const char* filename, std::vector<double> &dist_vector) const;
  unsigned int getRandomMultiplicity(void);
  void addHitsToTriggerEvent(TriggerEvent& e);
  void removeInactiveHits(void);
};



#endif
