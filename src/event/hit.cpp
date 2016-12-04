#include "hit.h"


Hit::Hit() {
  mChipId = 0;
  PixelData::mCol = 0;
  PixelData::mRow = 0;
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
  , PixelData(h.mCol, h.mRow)
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
  return (mChipId == rhs.mChipId) &&
    (mCol == rhs.mCol) &&
    (mRow == rhs.mRow);
}

bool Hit::operator>(const Hit& rhs) const
{
  bool retval = false;

  if(mChipId < rhs.mChipId)
    retval = false;
  else if(mChipId > rhs.mChipId)
    retval = true;
  else {
    if(mCol < rhs.mCol)
      retval = false;
    else if(mCol > rhs.mCol)
      retval = true;
    else {
      if(mRow < rhs.mRow)
        retval = false;
      else if(mRow > rhs.mRow)
        retval = true;
      else
        retval = false;
    }
  }

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
  mCol = rhs.mCol;
  mRow = rhs.mRow;
  mDeadTimeCounter = rhs.mDeadTimeCounter;
  mActiveTimeCounter = rhs.mActiveTimeCounter;
  return *this;
}
