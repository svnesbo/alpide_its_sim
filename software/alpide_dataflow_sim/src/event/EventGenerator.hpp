/**
 * @file   EventGenerator.hpp
 * @author Simon Voigt Nesbo
 * @date   December 22, 2016
 * @brief  A simple event generator for Alpide SystemC simulation model.
 */

///@defgroup event_generation Event Generation
///@{
#ifndef EVENT_GENERATOR_HPP
#define EVENT_GENERATOR_HPP

#include "EventFrame.hpp"
#include "EventXML.hpp"

// Ignore warnings about use of auto_ptr in SystemC library
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <systemc.h>
#pragma GCC diagnostic pop

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
class EventGenerator : sc_core::sc_module
{
public: // SystemC signals
  sc_in<bool> s_strobe_in;
  sc_in_clk s_clk_in;
  sc_event_queue_port E_event_frame_available;

  /// Active for one clock pulse every time we have a "physics event".
  /// Not really used for anything, just to indicate physics events in waveforms
  sc_out<bool> s_physics_event_out;

private:
  /// This is the event frame queue (ie. the hits that
  /// occur between a strobe, which are fed to the Alpide chips).
  /// Each Alpide chip has its own queue (corresponding to an index in the vector).
  std::vector<std::queue<EventFrame*> > mEventQueue;

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

  /// Total number of physics and event frames generated.
  int mPhysicsEventCount = 0;
  int mEventFrameIdCount = 0;

  /// Time of the last physics event that was generated.
  int64_t mLastPhysicsEventTimeNs = 0;

  /// Time of the last event frame that was generated (time of last strobe)
  /// Will not be updated if trigger was filtered out.
  int64_t mLastEventFrameStartTimeNs = 0;
  int64_t mLastEventFrameEndTimeNs = 0;

  bool mStrobeActive = false;

  /// Start time of next event frame (start time recorded on STROBE rising edge).
  /// Event actually created and hits assigned to it on STROBE falling edge.
  int64_t mNextEventFrameStartTimeNs = 0;

  /// Used by getNextEventFrame() so it doesn't have to start iterating from the
  /// beginning of the event queue vector each time it is called.
  /// Also used by removeOldestEvent().
  int mNextEventFrameChipId = 0;

  int mPixelDeadTime;
  int mPixelActiveTime;

  /// Minimum time between two triggers/events. Triggers/events that come sooner than this will
  /// be filtered out (but their hits will still be stored).
  int mTriggerFilterTimeNs;
  bool mTriggerFilteringEnabled = false;

  bool mContinuousMode = false;

  ///@todo This is currently used.. remove or update code that uses it..
  std::string mDataPath = "data";

  std::string mOutputPath;
  bool mWriteEventsToDisk = false;

  bool mCreateCSVFile = true;
  std::ofstream mPhysicsEventsCSVFile;
  std::ofstream mEventFramesCSVFile;

  int mRandomSeed;

  EventXML mMonteCarloEvents;

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

  bool mRandomHitGeneration;
  int mHitMultiplicityGaussAverage;
  int mHitMultiplicityGaussDeviation;

  void calculateAverageCrossingRate(void);
  void eventMemoryCountLimiter(void);

public:
  EventGenerator(sc_core::sc_module_name name,
                 const QSettings* settings,
                 std::string output_path);
  ~EventGenerator();
  std::shared_ptr<EventFrame> generateNextEventFrame(uint64_t event_start,
                                                     uint64_t event_end,int chip_id);
  const EventFrame& getNextEventFrame(void);
  void setBunchCrossingRate(int rate_ns);
  void setRandomSeed(int seed);
  void initRandomNumGenerator(void);
  void setPath(const std::string& path) {mDataPath = path;}
  void enableWriteToDisk(void) {mWriteEventsToDisk = true;}
  void disableWriteToDisk(void) {mWriteEventsToDisk = false;}
  void setNumEventsInMemAllowed(int n);
  int getTriggerFilterTime(void) const {return mTriggerFilterTimeNs;}
  int getEventsInMem(void) const {return mEventQueue.size();}
  int getPhysicsEventCount(void) const {return mPhysicsEventCount;}
  int getEventFrameCount(void) const {return mEventFrameIdCount;}
  void removeOldestEvent(void);
  void physicsEventMethod(void);
  //void eventFrameProcess(void);

private:

  uint64_t generateNextPhysicsEvent(void);
  void readDiscreteDistributionFile(const char* filename, std::vector<double> &dist_vector) const;
  void scaleDiscreteDistribution(std::vector<double> &dist_vector, double new_mean_value);
  unsigned int getRandomMultiplicity(void);
  void addHitsToEventFrame(EventFrame& e);
  void removeInactiveHits(void);
};


#endif
///@}
