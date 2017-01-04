/**
 * @file   pixel_col.h
 * @Author Simon Voigt Nesbo
 * @date   November 27, 2016
 * @brief  Pixel column, double column, and priority encoder classes
 *
 * Detailed description of file.
 */

#ifndef PIXEL_COL_H
#define PIXEL_COL_H


#include "alpide_constants.h"
#include <set>
#include <ostream>


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

public:
  PixelData(int col = 0, int row = 0) : mCol(col), mRow(row) {}
  PixelData(const PixelData& p) : mCol(p.mCol), mRow(p.mRow) {}
  int getCol(void) const {return mCol;}
  int getRow(void) const {return mRow;}
  void setCol(const int col) {mCol = col;}
  void setRow(const int row) {mRow = row;}
  bool operator==(const PixelData& rhs) const;
  bool operator>(const PixelData& rhs) const;
  bool operator<(const PixelData& rhs) const;
  bool operator>=(const PixelData& rhs) const;
  bool operator<=(const PixelData& rhs) const;
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
  bool operator()(const PixelData &leftIn, const PixelData &rightIn)
    {
      if(leftIn.mRow < rightIn.mRow)
        return true;
      else if(leftIn.mRow > rightIn.mRow)
        return false;      
      else { /* leftIn.mRow == rightIn.mRow */
        // Even row
        if((leftIn.mRow % 2) == 0)
          return (leftIn.mCol < rightIn.mCol);

        // Odd row
        else
          return (leftIn.mCol > rightIn.mCol);
      }
    }
};



class PixelDoubleColumn
{
private:
  std::set<PixelData, PixelPriorityEncoder> pixelColumn;
public:
  void setPixel(unsigned int col_num, unsigned int row_num);
  bool inspectPixel(unsigned int col_num, unsigned int row_num);
  PixelData readPixel(void);
  unsigned int pixelHitsRemaining(void);
};


#endif
