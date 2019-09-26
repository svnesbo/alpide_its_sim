/**
 * @file   EventGenPCT.hpp
 * @author Simon Voigt Nesbo
 * @date   November 14, 2018
 * @brief  A simple event generator for PCT simulation with Alpide SystemC simulation model.
 */

///@defgroup event_generation Event Generation
///@{
#ifndef EVENT_GEN_PCT_HPP
#define EVENT_GEN_PCT_HPP

#include <fstream>
#include <boost/random/poisson_distribution.hpp>
#include "Detector/PCT/PCTDetectorConfig.hpp"
#include "EventGenBase.hpp"
#ifdef ROOT_ENABLED
#include "EventRootPCT.hpp"
#endif


using std::uint64_t;

///@brief   A simple event generator for PCT simulation with Alpide SystemC simulation model.
///@details ...
class EventGenPCT : public EventGenBase
{
private:
  std::vector<std::shared_ptr<PixelHit>> mEventHitVector;

#ifdef ROOT_ENABLED
  EventRootPCT* mMCEvents = nullptr;
#endif

  PCT::PCTDetectorConfig mConfig;

  double mSingleChipDetectorArea;

  unsigned int mNumStavesPerLayer;

  unsigned int mRandomParticleCountMean_per_second;
  double mRandomBeamStdDev_mm;

  double mBeamCenterCoordX_mm;
  double mBeamCenterCoordY_mm;

  double mBeamStartCoordX_mm;
  double mBeamStartCoordY_mm;
  double mBeamEndCoordX_mm;
  double mBeamEndCoordY_mm;

  double mBeamStep_mm;
  double mBeamTimePerStep_us;

  int mBeamStepCounter = 0;

  /// Beam is moving to the right when true
  bool mBeamDirectionRight = true;

  /// Indicates that the beam has reached the desired end coords,
  /// and that the simulation should stop
  bool mBeamEndCoordsReached = false;

  unsigned int mEventTimeFrameLength_ns;

  std::ofstream mPCTEventsCSVFile;

  boost::random::mt19937 mRandParticleCountGen;
  boost::random::mt19937 mRandHitCoordsXGen;
  boost::random::mt19937 mRandHitCoordsYGen;

  boost::random::normal_distribution<double> *mRandParticlesPerEventFrameDist;
  boost::random::normal_distribution<double> *mRandHitXDist, *mRandHitYDist;

  void initCsvEventFileHeader(const QSettings* settings);
  void addCsvEventLine(uint64_t time_ns,
                       uint64_t particle_count_total,
                       uint64_t pixel_hit_count_total,
                       std::map<unsigned int, unsigned int> &chip_pixel_hits,
                       std::map<unsigned int, unsigned int> &layer_pixel_hits);
  void initRandomHitGen(const QSettings* settings);
  void initMonteCarloHitGen(const QSettings* settings);
  void generateRandomEventData(unsigned int &particle_count_out,
                               unsigned int &pixel_hit_count_out,
                               std::map<unsigned int, unsigned int> &chip_pixel_hits,
                               std::map<unsigned int, unsigned int> &layer_pixel_hits);
  bool generateMonteCarloEventData(unsigned int &particle_count_out,
                                   unsigned int &pixel_hit_count_out,
                                   std::map<unsigned int, unsigned int> &chip_pixel_hits,
                                   std::map<unsigned int, unsigned int> &layer_pixel_hits);
  bool generateEvent(void);
  void updateBeamPosition(void);
  void physicsEventMethod(void);

public:
  EventGenPCT(sc_core::sc_module_name name,
              const QSettings* settings,
              const PCT::PCTDetectorConfig& config,
              std::string output_path);
  ~EventGenPCT();
  void initRandomNumGenerators(const QSettings* settings);
  void stopEventGeneration(void);
  bool getBeamEndCoordsReached(void) const {return mBeamEndCoordsReached;}
  double getBeamCenterCoordX(void) const {return mBeamCenterCoordX_mm;}
  double getBeamCenterCoordY(void) const {return mBeamCenterCoordY_mm;}

  const std::vector<std::shared_ptr<PixelHit>>& getTriggeredEvent(void) const;
  const std::vector<std::shared_ptr<PixelHit>>& getUntriggeredEvent(void) const;
};


#endif
///@}
