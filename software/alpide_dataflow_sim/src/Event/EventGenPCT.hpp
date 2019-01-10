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

using std::uint64_t;

///@brief   A simple event generator for PCT simulation with Alpide SystemC simulation model.
///@details ...
class EventGenPCT : public EventGenBase
{
private:
  std::vector<std::shared_ptr<PixelHit>> mEventHitVector;

  EventBase* mMCEvents = nullptr;

  double mSingleChipDetectorArea;

  double mBeamCenterCoordX_mm;
  double mBeamCenterCoordY_mm;

  double mBeamSpeedX_mm_per_us;
  double mBeamSpeedY_mm_per_us;

  unsigned int mEventTimeFrameLength_ns;

  void generateRandomEventData(unsigned int &event_pixel_hit_count,
                               std::map<unsigned int, unsigned int> &chip_hits,
                               std::map<unsigned int, unsigned int> &layer_hits);
  void generateMonteCarloEventData(unsigned int &event_pixel_hit_count,
                                   std::map<unsigned int, unsigned int> &chip_hits,
                                   std::map<unsigned int, unsigned int> &layer_hits);
  uint64_t generateNextEvent(void);
  void eventMethod(void);

public:
  EventGenPCT(sc_core::sc_module_name name,
                 const QSettings* settings,
                 std::string output_path);
  ~EventGenPCT();
  void initRandomNumGenerators(void);

  const std::vector<std::shared_ptr<PixelHit>>& getTriggeredEvent(void);
  const std::vector<std::shared_ptr<PixelHit>>& getUntriggeredEvent(void);
};


#endif
///@}
