/**
 * @file   build_detector.h
 * @Author Simon Voigt Nesbo
 * @date   December 5, 2016
 * @brief  Header file ITS detector class
 *
 * Detailed description of file.
 */

#ifndef BUILD_DETECTOR_H
#define BUILD_DETECTOR_H

#include "../event/event.h"
#include <set>

class ITSDetector {
private:
  std::set<int> mLayerConfig;

  
public:
  ITSDetector();
  ITSDetector(std::set<int> layer_configuration);
  void feedEventsToChips(Event& e);
  void buildDetector(void);
}


#endif
