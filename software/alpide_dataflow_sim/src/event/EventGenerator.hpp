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
  sc_event E_physics_event;

  /// Active for one clock pulse every time we have a "physics event".
  /// Not really used for anything, just to indicate physics events in waveforms
  sc_out<bool> s_physics_event_out;

private:
  std::vector<Hit> mHitVector;

  int mNumChips;

  int mBunchCrossingRateNs;

  int mAverageEventRateNs;

  /// Total number of physics and event frames generated.
  int mPhysicsEventCount = 0;

  /// Time of the last physics event that was generated.
  int64_t mLastPhysicsEventTimeNs = 0;

  int mPixelDeadTime;
  int mPixelActiveTime;

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

  bool mRandomHitGeneration;
  int mHitMultiplicityGaussAverage;
  int mHitMultiplicityGaussDeviation;

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

public:
  EventGenerator(sc_core::sc_module_name name,
                 const QSettings* settings,
                 std::string output_path);
  ~EventGenerator();
  const EventFrame& getNextEventFrame(void);
  void setBunchCrossingRate(int rate_ns);
  void setRandomSeed(int seed);
  void initRandomNumGenerator(void);
  void setPath(const std::string& path) {mDataPath = path;}
  void enableWriteToDisk(void) {mWriteEventsToDisk = true;}
  void disableWriteToDisk(void) {mWriteEventsToDisk = false;}
  int getPhysicsEventCount(void) const {return mPhysicsEventCount;}
  void physicsEventMethod(void);

private:
  uint64_t generateNextPhysicsEvent(void);
  void readDiscreteDistributionFile(const char* filename,
                                    std::vector<double> &dist_vector) const;
  void scaleDiscreteDistribution(std::vector<double> &dist_vector,
                                 double new_mean_value);
  unsigned int getRandomMultiplicity(void);
  void calculateAverageCrossingRate(void);
  void eventMemoryCountLimiter(void);
};


#endif
///@}
