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


/**
   @brief A struct that indicates a hit in a region, at the pixel identified by the col and row
   variables. For each hit an object of this type will be inserted into the std::set container 
   in regionDataVector. For the pixels that don't have hits there will not be an object of this
   type inserted.
   Column should be 0 or 1. Row can be any value from 0 to N_PIXEL_ROWS-1.
*/
class PixelData
{
  int col;
  int row;
    
  PixelData(int colIn = 0, int rowIn = 0) :col(colIn) ,row(rowIn) {}
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
  /**
     @brief Overloaded () function, allows the std::set to use this function to compare two
     PixelData classes in the set, and determine which of them should come first when sorting them.
     @todo Fix prioritization here!
  */
  bool operator()(const PixelData &leftIn, const PixelData &rightIn)
    {
      if(leftIn.col/2 != rightIn.col/2)
      {
        return (leftIn.col < rightIn.col);
      }
      else if(leftIn.row == rightIn.row)
      {
        return (leftIn.col < rightIn.col);
      }
      else
      {
        return (leftIn.row < rightIn.row);
      }
    }
};



class PixelDoubleColumn
{
private:
  std::set<PixelData, PixelPriorityEncoder> pixelMEBColumns[N_MULTI_EVENT_BUFFERS];
  unsigned int strobe;
  unsigned int memsel;
public:
  setPixel(unsigned int col_num, unsigned int row_num);
  PixelData readPixel(void);
  unsigned int pixelsHitsRemaining(void);
};


#endif PIXEL_COL_H
