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

#include "EventGenBase.hpp"
#include "EventBase.hpp"
#include "../ITS/ITS_constants.hpp"
#include <fstream>
#include <boost/random/poisson_distribution.hpp>

using std::uint64_t;

///@brief   A simple event generator for PCT simulation with Alpide SystemC simulation model.
///@details ...
class EventGenPCT : public EventGenBase
{
private:
  std::vector<std::shared_ptr<PixelHit>> mEventHitVector;

  EventBase* mMCEvents = nullptr;

  double mSingleChipDetectorArea;

  unsigned int mNumLayers;
  unsigned int mNumStavesPerLayer;

  unsigned int mRandomParticleCountMean_per_second;
  double mRandomBeamStdDev_mm;

  double mBeamCenterCoordX_mm;
  double mBeamCenterCoordY_mm;

  double mBeamStartCoordX_mm;
  double mBeamStartCoordY_mm;
  double mBeamEndCoordX_mm;
  double mBeamEndCoordY_mm;

  double mBeamSpeedX_mm_per_us;
  double mBeamStepY_mm;

  /// Beam is moving to the right when true
  bool mBeamDirectionRight = true;

  unsigned int mEventTimeFrameLength_ns;

  uint64_t mPhysicsEventCount = 0;

  std::ofstream mPhysicsEventsCSVFile;

  boost::random::mt19937 mRandParticleCountGen;
  boost::random::mt19937 mRandHitCoordsXGen;
  boost::random::mt19937 mRandHitCoordsYGen;

  boost::random::poisson_distribution<uint32_t, double> *mRandParticlesPerEventFrameDist;
  boost::random::normal_distribution<double> *mRandHitXDist, *mRandHitYDist;

  void initCsvEventFileHeader(const QSettings* settings);
  void addCsvEventLine(uint64_t t_delta,
                       unsigned int event_pixel_hit_count,
                       std::map<unsigned int, unsigned int> &chip_hits,
                       std::map<unsigned int, unsigned int> &layer_hits);
  void initRandomHitGen(const QSettings* settings);
  void initMonteCarloHitGen(const QSettings* settings);
  void generateRandomEventData(unsigned int &event_pixel_hit_count,
                               std::map<unsigned int, unsigned int> &chip_hits,
                               std::map<unsigned int, unsigned int> &layer_hits);
  void generateMonteCarloEventData(unsigned int &event_pixel_hit_count,
                                   std::map<unsigned int, unsigned int> &chip_hits,
                                   std::map<unsigned int, unsigned int> &layer_hits);
  void generateEvent(void);
  void updateBeamPosition(void);
  void physicsEventMethod(void);

public:
  EventGenPCT(sc_core::sc_module_name name,
                 const QSettings* settings,
                 std::string output_path);
  ~EventGenPCT();
  void initRandomNumGenerators(const QSettings* settings);
  void stopEventGeneration(void);

  const std::vector<std::shared_ptr<PixelHit>>& getTriggeredEvent(void) const;
  const std::vector<std::shared_ptr<PixelHit>>& getUntriggeredEvent(void) const;
};


#endif
///@}
