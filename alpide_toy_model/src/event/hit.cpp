/**
 * @file   hit.h
 * @Author Simon Voigt Nesbo
 * @date   December 12, 2016
 * @brief  Source file for PixelData and Hit classes. These classes hold the
 *         coordinates for a discrete hit in the Alpide chip, along with 
 *         information about when the hit occured and so on (used to model
 *         the analog shaping time etc. in pixel front end).
 *
 * Detailed description of file.
 */

#include "hit.h"


Hit::Hit()
  : PixelData(0, 0)
{
  mChipId = 0;
  mDeadTimeCounter = 0;
  mActiveTimeCounter = 0;
}

Hit::Hit(int chip_id, int col, int row, int dead_time_ns, int active_time_ns)
  : mChipId(chip_id)
  , PixelData(col, row)
  , mDeadTimeCounter(dead_time_ns)
  , mActiveTimeCounter(active_time_ns)
{
}

Hit::Hit(const Hit& h)
  : mChipId(h.mChipId)
  , PixelData(h)
  , mDeadTimeCounter(h.mDeadTimeCounter)
  , mActiveTimeCounter(h.mActiveTimeCounter)
{
}

bool Hit::isActive(void)
{
  return (mDeadTimeCounter == 0 && mActiveTimeCounter > 0);
}

void Hit::decreaseTimers(int time_ns)
{
  if(mDeadTimeCounter > 0) {
    mDeadTimeCounter -= time_ns;
    if(mDeadTimeCounter < 0) {
      mActiveTimeCounter += mDeadTimeCounter;
      mDeadTimeCounter = 0;
    }
  } else {
    mActiveTimeCounter -= time_ns;
  }

  if(mActiveTimeCounter < 0)
    mActiveTimeCounter = 0;
}

int Hit::timeLeft(void) const
{
  return mDeadTimeCounter + mActiveTimeCounter;
}
  
bool Hit::operator==(const Hit& rhs) const
{
  const PixelData& pixel_lhs = dynamic_cast<const PixelData&>(*this);
  const PixelData& pixel_rhs = dynamic_cast<const PixelData&>(rhs);

  // Hits are equal if chip ids and hit coords match
  return (mChipId == rhs.mChipId) && (pixel_lhs == pixel_rhs);
  // return (mChipId == rhs.mChipId) &&
  //   (mCol == rhs.mCol) &&
  //   (mRow == rhs.mRow);
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
  mDeadTimeCounter = rhs.mDeadTimeCounter;
  mActiveTimeCounter = rhs.mActiveTimeCounter;
  return *this;
}
