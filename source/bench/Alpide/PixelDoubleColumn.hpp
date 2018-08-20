/**
 * @file   pixel_col.h
 * @author Simon Voigt Nesbo
 * @date   November 27, 2016
 * @brief  Pixel column, double column, and priority encoder classes
 *
 * Detailed description of file.
 */


///@addtogroup pixel_stuff
///@{
#ifndef PIXEL_COL_H
#define PIXEL_COL_H

#include "alpide_constants.hpp"
#include "PixelReadoutStats.hpp"
#include <set>
#include <ostream>
#include <memory>


/**
   @brief A struct that indicates a hit in a region, at the pixel identified by the col and row
   variables. For each hit an object of this type will be inserted into the std::set container
   in regionDataVector. For the pixels that don't have hits there will not be an object of this
   type inserted.
   Column should be 0 or 1. Row can be any value from 0 to N_PIXEL_ROWS-1.
*/
class PixelData
{
  friend class PixelPriorityEncoder;

private:
  int mCol;
  int mRow;
  unsigned int mReadoutCount = 0;
  std::shared_ptr<PixelReadoutStats> mPixelReadoutStats;

public:
  ///@brief Constructor for PixelData base class, which contains coordinates for a pixel hit
  ///@param[in] col Column in ALPIDE pixel matrix
  ///@param[in] row Column in ALPIDE pixel matrix
  ///@param[in] hit_stats Shared pointer to HitStats object. When the Hit object is descructed,
  ///           readout counters in HitStats will be increased. hit_stats has a default
  ///           assignment and can be omitted if not used.
  PixelData(int col = 0, int row = 0,
            const std::shared_ptr<PixelReadoutStats> &pix_stats =
            std::shared_ptr<PixelReadoutStats>)
    : mCol(col), mRow(row) {}

  ///@brief Constructor for PixelData base class, which contains coordinates for a pixel
  ///hit. This constructor calculates the col/row from priority encoder address
  ///@param[in] region Region number in ALPIDE pixel matrix
  ///@param[in] pri_enc Priority encoder number (ie. double column number) within region
  ///@param[in] addr Address of hit within priority encoder
  ///@param[in] hit_stats Shared pointer to HitStats object. When the Hit object is descructed,
  ///           readout counters in HitStats will be increased. hit_stats has a default
  ///           assignment and can be omitted if not used.
  PixelData(int region, int pri_enc, int addr,
            const std::shared_ptr<PixelReadoutStats> &pix_stats =
            std::shared_ptr<PixelReadoutStats>);

  PixelData(const PixelData& p)
    : mCol(p.mCol),
      mRow(p.mRow),
      mPixelReadoutStats(p.mPixelReadoutStats) {}

  ~PixelData() {
    if(mPixelReadoutStats) {
      mPixelReadoutStats->addReadoutCount(mReadoutCount);
    }
  }

  bool operator==(const PixelData& rhs) const;
  bool operator>(const PixelData& rhs) const;
  bool operator<(const PixelData& rhs) const;
  bool operator>=(const PixelData& rhs) const;
  bool operator<=(const PixelData& rhs) const;
  int getCol(void) const {return mCol;}
  int getRow(void) const {return mRow;}
  void setCol(const int col) {mCol = col;}
  void setRow(const int row) {mRow = row;}

  ///@brief Get the "address" of this pixel within it's double column, that is
  /// the priority that this pixel has in the priority encoder would.
  ///@return Pixel's priority encoder address/priority
  unsigned int getPriEncPixelAddress(void) const {return (mRow<<1) + ((mCol&1)^(mRow&1));}

  ///@brief Get the priority encoder that this pixel (column) belongs to,
  ///       within the column's region. Hardcoded for 16 double columns per region
  ///@return Priority encoder number.
  unsigned int getPriEncNumInRegion(void) const {return (mCol>>1) & 0x0F;}

  ///@brief Get the number of times this pixel hit has been read out
  ///@return Readout count
  inline unsigned int getReadoutCount(void) const {
    return mReadoutCount;
  }

  ///@brief Increase the number of times this pixel was read out
  inline void increaseReadoutCount(void) {
    mReadoutCount++;
  }

  inline void setPixelReadoutStatsObj(const std::shared_ptr<PixelStats> &pix_stats) {
    mPixelReadoutStats = pix_stats;
  }
};

const PixelData NoPixelHit(-1,-1);

/**
   @brief Comparator class/function for use with the PixelData class in the
   std::set container, which allows the container to sort the PixelData
   entries in a meaningful way. The picture below is from the ALPIDE operations
   manual, and shows:
   - Left: 512 rows x 1024 columns of pixels, divided into 32 regions
   - Middle: 32 columns (16 double columns) x 512 rows in a region
   - Right: Index/numbering/address of pixels within a double column, and the
   priority encoder between the columns. The priority encoder starts with the pixel
   that has the lowest address, and prioritizes them in increasing order.

   The regionDataVector, which is declared as @b std::vector<std::set<PixelData, PixelComparer> @b > @b regionDataVector;
   attempts to implement the priority encoder to reflect what is on the ALPIDE chip.
   Only pixels that have hits will be stored in the set, the pixels in the set are read
   out in increasing order (starting with index 0), and the PixelComparer type implements
   the actual prioritization of the pixels in the set.
   @image html regions-columns-indexes.png
   @image latex regions-columns-indexes.png "Overview of how regions and columns are indexed, and how pixels are indexed in double columns, in the Alpide chip."
*/
class PixelPriorityEncoder
{
public:
  /**
     @brief Overloaded () function, allows the std::set to use this function to compare two
     PixelData classes in the set, and determine which of them should come first when sorting them.
     The prioritization works like this:
     - Lower rows prioritized first
     - For even rows, the column 0 pixel comes first
     - For odd rows, the column 1 pixel comes first
     @param leftIn Left side argument
     @param rightIn Right side argument
     @return True if leftIn has highest priority, false if rightIn has higest priority
  */
  bool operator()(const std::shared_ptr<PixelData> &leftIn,
                  const std::shared_ptr<PixelData> &rightIn)
    {
      if(leftIn->mRow < rightIn->mRow)
        return true;
      else if(leftIn->mRow > rightIn->mRow)
        return false;
      else { /* leftIn.mRow == rightIn.mRow */
        // Even row
        if((leftIn->mRow % 2) == 0)
          return (leftIn->mCol < rightIn->mCol);

        // Odd row
        else
          return (leftIn->mCol > rightIn->mCol);
      }
    }
};



class PixelDoubleColumn
{
private:
  std::set<std::shared_ptr<PixelData>, PixelPriorityEncoder> pixelColumn;
public:
  void setPixel(unsigned int col_num, unsigned int row_num);
  void setPixel(const std::shared_ptr<PixelData> &pixel);
  void clear(void);
  bool inspectPixel(unsigned int col_num, unsigned int row_num);
  std::shared_ptr<PixelData> readPixel(void);
  unsigned int pixelHitsRemaining(void);
};


#endif
///@}
