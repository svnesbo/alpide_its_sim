/**
 * @file   EventGenITS.hpp
 * @author Simon Voigt Nesbo
 * @date   November 14, 2018
 * @brief  A simple event generator for ITS simulation with Alpide SystemC simulation model.
 */

///@defgroup event_generation Event Generation
///@{
#ifndef EVENT_GEN_ITS_HPP
#define EVENT_GEN_ITS_HPP

#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/exponential_distribution.hpp>
#include <boost/random/discrete_distribution.hpp>
#include "Detector/PCT/PCTDetectorConfig.hpp"
#include "Detector/ITS/ITSDetectorConfig.hpp"
#include "EventGenBase.hpp"
#include "EventBaseDiscrete.hpp"

#ifdef ROOT_ENABLED
#include "EventRootFocal.hpp"
#endif


///@brief   A simple event generator for ITS simulation with Alpide SystemC simulation model.
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
class EventGenITS : public EventGenBase
{
private:
  std::vector<std::shared_ptr<PixelHit>> mEventHitVector;
  std::vector<std::shared_ptr<PixelHit>> mQedNoiseHitVector;

  int mBunchCrossingRate_ns;
  int mAverageEventRate_ns;

  EventBaseDiscrete* mMCPhysicsEvents = nullptr;
  EventBaseDiscrete* mMCQedNoiseEvents = nullptr;

#ifdef ROOT_ENABLED
  EventRootFocal* mFocalEvents = nullptr;
#endif

  Detector::DetectorConfigBase mDetectorConfig;

  double mHitDensities[ITS::N_LAYERS];
  double mDetectorArea[ITS::N_LAYERS];
  double mHitAverage[ITS::N_LAYERS];
  double mMultiplicityScaleFactor[ITS::N_LAYERS];

  double mSingleChipHitDensity;
  double mSingleChipDetectorArea;
  double mSingleChipHitAverage;
  double mSingleChipMultiplicityScaleFactor;

  boost::random::mt19937 mRandHitGen;
  boost::random::mt19937 mRandHitMultiplicityGen;
  boost::random::mt19937 mRandEventTimeGen;

  /// Uniform distribution used generating hit coordinates
  boost::random::uniform_int_distribution<int> *mRandHitChipX, *mRandHitChipY;
  boost::random::uniform_int_distribution<int> *mRandStave[ITS::N_LAYERS];
  boost::random::uniform_int_distribution<int> *mRandSubStave[ITS::N_LAYERS];
  boost::random::uniform_int_distribution<int> *mRandModule[ITS::N_LAYERS];
  boost::random::uniform_int_distribution<int> *mRandChipID[ITS::N_LAYERS];

  /// Choice of discrete distribution (based on discrete list of N_hits vs Probability),
  /// or gaussian distribution.
  boost::random::discrete_distribution<> *mRandHitMultiplicity;

  /// Exponential distribution used for time between events
  boost::random::exponential_distribution<double> *mRandEventTime;

  std::ofstream mPhysicsEventsCSVFile;

  void generateRandomEventData(uint64_t event_time_ns,
                               unsigned int &event_pixel_hit_count,
                               std::map<unsigned int, unsigned int> &chip_hits,
                               std::map<unsigned int, unsigned int> &layer_hits);

  void generateMonteCarloEventData(uint64_t event_time_ns,
                                   unsigned int &event_pixel_hit_count,
                                   std::map<unsigned int, unsigned int> &chip_hits,
                                   std::map<unsigned int, unsigned int> &layer_hits);

  uint64_t generateNextPhysicsEvent(void);
  void generateNextQedNoiseEvent(uint64_t event_time_ns);
  void readDiscreteDistributionFile(const char* filename,
                                    std::vector<double> &dist_vector) const;
  void initRandomHitGen(const QSettings* settings);
  void initRandomNumGen(const QSettings* settings);
  void initMonteCarloHitGen(const QSettings* settings);
  void initCsvEventFileHeader(const QSettings* settings);
  void addCsvEventLine(uint64_t t_delta,
                       unsigned int event_pixel_hit_count,
                       std::map<unsigned int, unsigned int> &chip_hits,
                       std::map<unsigned int, unsigned int> &layer_hits);
  double normalizeDiscreteDistribution(std::vector<double> &dist_vector);
  unsigned int getRandomMultiplicity(void);
  void physicsEventMethod(void);
  void qedNoiseEventMethod(void);

public:
  EventGenITS(sc_core::sc_module_name name,
              Detector::DetectorConfigBase config,
              const QSettings* settings,
              std::string output_path);
  ~EventGenITS();
  void setBunchCrossingRate(int rate_ns);
  void stopEventGeneration(void);

  const std::vector<std::shared_ptr<PixelHit>>& getTriggeredEvent(void) const;
  const std::vector<std::shared_ptr<PixelHit>>& getUntriggeredEvent(void) const;
};

#endif
///@}
