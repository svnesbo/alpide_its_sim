/**
 * @file   hit.h
 * @Author Simon Voigt Nesbo
 * @date   December 12, 2016
 * @brief  Header file for PixelData and Hit classes. These classes hold the
 *         coordinates for a discrete hit in the Alpide chip, along with 
 *         information about when the hit occured and so on (used to model
 *         the analog shaping time etc. in pixel front end).
 *
 * Detailed description of file.
 */

#ifndef HIT_H
#define HIT_H

#include "../alpide/pixel_col.h"

const int pixel_shaping_dead_time_ns = 200;
const int pixel_shaping_active_time_ns = 6000;

class Hit : public PixelData
{
private:
  int mChipId;
  int mDeadTimeCounter;
  int mActiveTimeCounter;
public:
  Hit();
  Hit(int chip_id, int col, int row,
      int dead_time_ns = pixel_shaping_dead_time_ns,
      int active_time_ns = pixel_shaping_active_time_ns);
  Hit(const Hit& h);
  bool isActive(void);
  void decreaseTimers(int time_ns);
  int timeLeft(void) const;  
  bool operator==(const Hit& rhs) const;
  bool operator>(const Hit& rhs) const;
  bool operator<(const Hit& rhs) const;
  bool operator>=(const Hit& rhs) const;
  bool operator<=(const Hit& rhs) const;
  
  Hit& operator=(const Hit& rhs);
  int getChipId(void) const {return mChipId;}
};


#endif
