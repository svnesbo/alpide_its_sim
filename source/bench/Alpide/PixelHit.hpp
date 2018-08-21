/**
 * @file   PixelHit.hpp
 * @author Simon Voigt Nesbo
 * @date   August 14, 2018
 * @brief  Class for a pixel hit
 *
 */
#ifndef PIXEL_HIT_HPP
#define PIXEL_HIT_HPP

#include "PixelReadoutStats.hpp"
#include <cstdint>
#include <algorithm>
#include <memory>

using std::uint64_t;

class PixelPriorityEncoder;

/**
   @brief A struct that indicates a hit in a region, at the pixel identified by the col and row
   variables.
*/
class PixelHit
{
  friend class PixelPriorityEncoder;

private:
  int mCol;
  int mRow;
  unsigned int mChipId;

  uint64_t mActiveTimeStartNs = 0;
  uint64_t mActiveTimeEndNs = 0;
  unsigned int mReadoutCount = 0;
  std::shared_ptr<PixelReadoutStats> mPixelReadoutStats;

public:
  PixelHit(int col = 0, int row = 0, unsigned int chip_id = 0,
           const std::shared_ptr<PixelReadoutStats> &readout_stats =
           nullptr);
  PixelHit(int region, int pri_enc, int addr, unsigned int chip_id = 0,
           const std::shared_ptr<PixelReadoutStats> &readout_stats =
           nullptr);
  PixelHit(const PixelHit& p);
  ~PixelHit();

  bool operator==(const PixelHit& rhs) const;
  bool operator>(const PixelHit& rhs) const;
  bool operator<(const PixelHit& rhs) const;
  bool operator>=(const PixelHit& rhs) const;
  bool operator<=(const PixelHit& rhs) const;
  int getCol(void) const;
  int getRow(void) const;
  unsigned int getChipId(void) const;
  void setCol(const int col);
  void setRow(const int row);
  void setChipId(const unsigned int chip_id);

  unsigned int getPriEncPixelAddress(void) const;
  unsigned int getPriEncNumInRegion(void) const;
  unsigned int getReadoutCount(void) const;
  void increaseReadoutCount(void);
  void setPixelReadoutStatsObj(const std::shared_ptr<PixelReadoutStats> &pix_stats);
  void setActiveTimeStart(uint64_t start_time_ns);
  void setActiveTimeEnd(uint64_t end_time_ns);
  uint64_t getActiveTimeStart(void) const;
  uint64_t getActiveTimeEnd(void) const;
  bool isActive(uint64_t time_now_ns) const;
  bool isActive(uint64_t strobe_start_time_ns, uint64_t strobe_end_time_ns) const;
};

const PixelHit NoPixelHit(-1,-1);


///@brief Constructor for PixelHit base class, which contains coordinates for a pixel hit
///@param[in] col Column in ALPIDE pixel matrix
///@param[in] row Column in ALPIDE pixel matrix
///@param[in] chip_id Chip ID (typically global chip ID, not in a stave, but can be used for whatever)
///@param[in] readout_stats Shared pointer to PixelReadoutStats object. When the PixelHit
///           object is destructed, readout counters in HitStats will be increased. hit_stats
///           has a default assignment and can be omitted if not used.
inline PixelHit::PixelHit(int col, int row, unsigned int chip_id,
                          const std::shared_ptr<PixelReadoutStats> &readout_stats)
  : mCol(col)
  , mRow(row)
  , mChipId(chip_id)
  , mPixelReadoutStats(readout_stats)
{
}


///@brief Constructor for PixelHit base class, which contains coordinates for a pixel
///hit. This constructor calculates the col/row from priority encoder address
///@param[in] region Region number in ALPIDE pixel matrix
///@param[in] pri_enc Priority encoder number (ie. double column number) within region
///@param[in] addr Address of hit within priority encoder
///@param[in] readout_stats Shared pointer to PixelReadoutStats object. When the PixelHit
///           object is destructed, readout counters in HitStats will be increased. hit_stats
///           has a default assignment and can be omitted if not used.
inline PixelHit::PixelHit(int region, int pri_enc, int addr, unsigned int chip_id,
                          const std::shared_ptr<PixelReadoutStats> &readout_stats)
  : mChipId(chip_id)
  , mPixelReadoutStats(readout_stats)
{
  mRow = addr >> 1;
  mCol = ((addr&1) ^ (mRow&1)); // LSB of column
  mCol = (region << 5) | (pri_enc << 1) | mCol;
}


inline PixelHit::PixelHit(const PixelHit& p)
  : mCol(p.mCol)
  , mRow(p.mRow)
  , mChipId(p.mChipId)
  , mPixelReadoutStats(p.mPixelReadoutStats)
{
}


inline PixelHit::~PixelHit()
{
  if(mPixelReadoutStats) {
    mPixelReadoutStats->addReadoutCount(mReadoutCount);
  }
}


///@brief Compare if two PixelHit objects are equal. Does not compare chip id.
inline bool PixelHit::operator==(const PixelHit& rhs) const
{
  return (this->mCol == rhs.mCol) && (this->mRow == rhs.mRow);
}


///@brief Compare if one PixelHit object is greater than another, based on priority encoder
///       readout order (which is read out first?). Does not compare chip id.
inline bool PixelHit::operator>(const PixelHit& rhs) const
{
  bool retval = false;

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

  return retval;
}


inline bool PixelHit::operator<(const PixelHit& rhs) const
{
  return rhs > *this;
}


inline bool PixelHit::operator>=(const PixelHit& rhs) const
{
  return !(*this < rhs);
}


inline bool PixelHit::operator<=(const PixelHit& rhs) const
{
  return !(*this > rhs);
}

inline int PixelHit::getCol(void) const
{
  return mCol;
}


inline int PixelHit::getRow(void) const
{
  return mRow;
}


inline unsigned int PixelHit::getChipId(void) const
{
  return mChipId;
}


inline void PixelHit::setCol(const int col)
{
  mCol = col;
}


inline void PixelHit::setRow(const int row)
{
  mRow = row;
}


inline void PixelHit::setChipId(const unsigned int chip_id)
{
  mChipId = chip_id;
}


///@brief Get the "address" of this pixel within it's double column, that is
/// the priority that this pixel has in the priority encoder would.
///@return Pixel's priority encoder address/priority
inline unsigned int PixelHit::getPriEncPixelAddress(void) const
{
  return (mRow<<1) + ((mCol&1)^(mRow&1));
}


///@brief Get the priority encoder that this pixel (column) belongs to,
///       within the column's region. Hardcoded for 16 double columns per region
///@return Priority encoder number.
inline unsigned int PixelHit::getPriEncNumInRegion(void) const
{
  return (mCol>>1) & 0x0F;
}

///@brief Get the number of times this pixel hit has been read out
///@return Readout count
inline unsigned int PixelHit::getReadoutCount(void) const
{
  return mReadoutCount;
}

///@brief Increase the number of times this pixel was read out
inline void PixelHit::increaseReadoutCount(void)
{
  mReadoutCount++;
}

inline void PixelHit::setPixelReadoutStatsObj(const std::shared_ptr<PixelReadoutStats> &pix_stats)
{
  mPixelReadoutStats = pix_stats;
}

inline void PixelHit::setActiveTimeStart(uint64_t start_time_ns)
{
  mActiveTimeStartNs = start_time_ns;
}

inline void PixelHit::setActiveTimeEnd(uint64_t end_time_ns)
{
  mActiveTimeEndNs = end_time_ns;
}

inline uint64_t PixelHit::getActiveTimeStart(void) const
{
  return mActiveTimeStartNs;
}

inline uint64_t PixelHit::getActiveTimeEnd(void) const
{
  return mActiveTimeEndNs;
}

///@brief Check if this hit is currently active (which is equivalent to when analog pulse shape is over threshold).
///@param time_now_ns Current simulation time (in nanoseconds).
///@return True if active, false if not.
inline bool PixelHit::isActive(uint64_t time_now_ns) const
{
  return (time_now_ns >= mActiveTimeStartNs) && (time_now_ns < mActiveTimeEndNs);
}

///@brief Check if this hit is active at any time during the specified
///       time duration (between strobe_start_time_ns and strobe_end_time_ns).
///@param strobe_start_time_ns Strobe start time
///@param strobe_end_time_ns Strobe end time
///@return True if active, false if not.
inline bool PixelHit::isActive(uint64_t strobe_start_time_ns, uint64_t strobe_end_time_ns) const
{
  // Check for two overlapping integer ranges:
  // http://stackoverflow.com/a/12888920
  return(std::max(strobe_start_time_ns, mActiveTimeStartNs) <=
         std::min(strobe_end_time_ns, mActiveTimeEndNs));
}


#endif
