/**
 * @file   hit.h
 * @author Simon Voigt Nesbo
 * @date   December 12, 2016
 * @brief  Header file for PixelData and Hit classes. These classes hold the coordinates for a discrete hit
 *         in the Alpide chip, along with information about when the hit is active (equivalent to when the 
 *         analog pulse out of the amplifier and shaping stage in the analog front end goes above the
 *         threshold).
 */

#ifndef HIT_H
#define HIT_H

///@todo Move PixelData class to its own file? Currently in pixel_col.h
#include "../alpide/pixel_col.h"
#include <cstdint>
#include <algorithm>

using std::int64_t;

class Hit : public PixelData
{
private:
  int mChipId;
  int64_t mActiveTimeStartNs;
  int64_t mActiveTimeEndNs;
public:
  Hit();
  Hit(int chip_id, int col, int row, int64_t time_now_ns, int dead_time_ns, int active_time_ns);
  Hit(int chip_id, int col, int row, int64_t time_active_start_ns, int64_t time_active_end_ns);
  Hit(const Hit& h);

  bool operator==(const Hit& rhs) const;
  bool operator>(const Hit& rhs) const;
  bool operator<(const Hit& rhs) const;
  bool operator>=(const Hit& rhs) const;
  bool operator<=(const Hit& rhs) const;
  Hit& operator=(const Hit& rhs);
  int getChipId(void) const {return mChipId;}
  int64_t getActiveTimeStart(void) const {return mActiveTimeStartNs;}
  int64_t getActiveTimeEnd(void) const {return mActiveTimeEndNs;}  

  ///@brief Check if this hit is currently active (which is equivalent to when analog pulse shape is over threshold).
  ///@param time_now_ns Current simulation time (in nanoseconds).
  ///@return True if active, false if not.
  inline bool isActive(int64_t time_now_ns) const {
    return (time_now_ns >= mActiveTimeStartNs) && (time_now_ns < mActiveTimeEndNs);
  }

  ///@brief Check if this hit is active at any time during the specified
  ///       time duration (between strobe_start_time_ns and strobe_end_time_ns).
  ///@param strobe_start_time_ns Strobe start time
  ///@param strobe_end_time_ns Strobe end time
  ///@return True if active, false if not.
  inline bool isActive(int64_t strobe_start_time_ns, int64_t strobe_end_time_ns) const {
    // Check for two overlapping integer ranges:
    // http://stackoverflow.com/a/12888920
    return
      std::max(strobe_start_time_ns, mActiveTimeStartNs) <=
      std::min(strobe_end_time_ns, mActiveTimeEndNs);
  }
};


#endif
