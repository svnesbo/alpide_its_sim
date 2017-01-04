/**
 * @file   hit.h
 * @author Simon Voigt Nesbo
 * @date   December 12, 2016
 * @brief  Source file for PixelData and Hit classes. These classes hold the coordinates for a discrete hit
 *         in the Alpide chip, along with information about when the hit is active (equivalent to when the 
 *         analog pulse out of the amplifier and shaping stage in the analog front end goes above the
 *         threshold).
 */

#include "hit.h"


Hit::Hit()
  : PixelData(0, 0)
  , mChipId(0)
  , mActiveTimeStartNs(0)
  , mActiveTimeEndNs(0)
{
}


///@brief Constructor that calculates active start and end times based on current simulation time, and
///       dead times and active times.
///@param chip_id Chip ID.
///@param col Column number.
///@param row Row number.
///@param time_now_ns Time (in nanoseconds) when this hit occured (ie. current simulation time).
///@param dead_time_ns Dead time (in nanoseconds) before the hit "becomes active". This is equivalent to the
///       time it takes for the analog signal to go above the threshold after a hit.
///@param active_time_ns Specifies (in nanoseconds) how long the hit stays active (ie. pixel is triggered)
///       after the dead time has passed. This is equivalent to the time the analog pulse into the
///       discriminator/comparator is over threshold.
Hit::Hit(int chip_id, int col, int row, int64_t time_now_ns, int dead_time_ns, int active_time_ns)
  : PixelData(col, row)
  , mChipId(chip_id)
{
  mActiveTimeStartNs = time_now_ns + dead_time_ns;
  mActiveTimeEndNs = time_now_ns + dead_time_ns + active_time_ns;
}


///@brief Constructor that takes active start and end times directly.
///@param chip_id Chip ID.
///@param col Column number.
///@param row Row number.
///@param time_active_start_ns Absolute simulation time (in nanoseconds) for when the hit becomes active, which
///       is equivalent to the analog signal going above the threshold after a hit.
///@param time_active_end_ns Absolute simulation time (in nanoseconds) for when the hit stops being active, which
///       is equivalent to when the analog signal goes below the threshold again after having been active.
Hit::Hit(int chip_id, int col, int row, int64_t time_active_start_ns, int64_t time_active_end_ns)
  : PixelData(col, row)
  , mChipId(chip_id)
  , mActiveTimeStartNs(time_active_start_ns)
  , mActiveTimeEndNs(time_active_end_ns)
      
{
}


Hit::Hit(const Hit& h)
  : PixelData(h)
  , mChipId(h.mChipId)
  , mActiveTimeStartNs(h.mActiveTimeStartNs)
  , mActiveTimeEndNs(h.mActiveTimeEndNs)
{
}

  
bool Hit::operator==(const Hit& rhs) const
{
  const PixelData& pixel_lhs = dynamic_cast<const PixelData&>(*this);
  const PixelData& pixel_rhs = dynamic_cast<const PixelData&>(rhs);

  // Hits are considered equal if chip ids and hit coords match,
  // active times are not taken into account in this comparison.
  return (mChipId == rhs.mChipId) && (pixel_lhs == pixel_rhs);
}

bool Hit::operator>(const Hit& rhs) const
{
  const PixelData& pixel_lhs = dynamic_cast<const PixelData&>(*this);
  const PixelData& pixel_rhs = dynamic_cast<const PixelData&>(rhs);  
  bool retval = false;

  if(mChipId < rhs.mChipId)
    retval = false;
  else if(mChipId > rhs.mChipId)
    retval = true;
  else
    retval = pixel_lhs > pixel_rhs;

  return retval;
}

bool Hit::operator<(const Hit& rhs) const
{
  return rhs > *this;
}

bool Hit::operator>=(const Hit& rhs) const
{
  return !(*this < rhs);
}

bool Hit::operator<=(const Hit& rhs) const
{
  return !(*this > rhs);
}    
  
Hit& Hit::operator=(const Hit& rhs)
{
  mChipId = rhs.mChipId;
  setCol(rhs.getCol());
  setRow(rhs.getRow());
  mActiveTimeStartNs = rhs.mActiveTimeStartNs;
  mActiveTimeEndNs = rhs.mActiveTimeEndNs;
  return *this;
}
