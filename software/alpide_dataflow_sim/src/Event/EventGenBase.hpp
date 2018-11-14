/**
 * @file   EventGenBase.hpp
 * @author Simon Voigt Nesbo
 * @date   November 14, 2018
 * @brief  Base class for event generator
 */

///@defgroup event_generation Event Generation
///@{
#ifndef EVENT_GEN_BASE_HPP
#define EVENT_GEN_BASE_HPP

#include "Alpide/PixelHit.hpp"

// Ignore warnings about use of auto_ptr in SystemC library
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <systemc.h>
#pragma GCC diagnostic pop

#include <QSettings>
#include <cstdint>
#include <vector>
#include <memory>
#include <string>

using std::uint64_t;

class EventGenBase
{
public: // SystemC signals
  /// This sc_event is used for events such as a collision in LHC, and can be
  /// used to initiate a trigger to the detectors. Hit data, which should be
  /// inputted to the detector, is available at the time of this sc_event.
  /// when calling getTriggeredEvent(). This needs to be done even if we are
  /// not triggering the detector on this event (such as ITS in continuous mode).
  sc_event E_triggered_event;

  /// This sc_event is used for "events" that happen continuously,
  /// and which are not triggered on,
  /// This sc_event is used for "events/hits" that happen continuously,
  /// such as QED background, noise, etc. Hit data, which should be
  /// inputted to the detector, is available at the time of this sc_event
  /// when calling getUntriggeredEvent().
  sc_event E_untriggered_event;

private:
  std::vector<std::shared_ptr<PixelHit>> mEventHitVector;
  std::vector<std::shared_ptr<PixelHit>> mQedNoiseHitVector;

  int mNumChips = 0;

  bool mSingleChipSimulation = false;
  bool mStopEventGeneration = false;
  bool mQedNoiseGenEnable = false;

  uint64_t mQedNoiseFeedRateNs = 0;
  uint64_t mQedNoiseEventRateNs = 0;

  /// Total number of triggered event frames generated.
  uint64_t mTriggeredEventCount = 0;

  /// Total number of untriggered event frames generated.
  uint64_t mUntriggeredEventCount = 0;

  int mPixelDeadTime;
  int mPixelActiveTime;

  std::string mOutputPath;

  int mRandomSeed;

  /// Readout stats objects for triggered data
  std::shared_ptr<PixelReadoutStats> mTriggeredReadoutStats = nullptr;

  /// Readout stats objects for untriggered data
  std::shared_ptr<PixelReadoutStats> mUntriggeredReadoutStats = nullptr;

  bool mRandomHitGeneration;

  /// Mean cluster size (pixels) per hit, used for random cluster generation
  double mRandClusterSizeMean;

  /// Mean cluster size deviation (pixels) per hit, used for random cluster generation
  double mRandClusterSizeDeviation;

public:
  EventGenBase(const QSettings* settings, std::string output_path);
  ~EventGenBase();
  virtual const std::vector<std::shared_ptr<PixelHit>>& getTriggeredEvent(void) const = 0;
  virtual const std::vector<std::shared_ptr<PixelHit>>& getUntriggeredEvent(void) const = 0;
  virtual void initRandomNumGenerators(void);
  void setRandomSeed(int seed);
  void setPath(const std::string& path) {mDataPath = path;}
  uint64_t getTriggeredEventCount(void) const {return mTriggeredEventCount;}
  uint64_t getUntriggeredEventCount(void) const {return mUntriggeredEventCount;}
  void stopEventGeneration(void);
  void writeSimulationStats(const std::string output_path) const;
};

#endif
///@}
