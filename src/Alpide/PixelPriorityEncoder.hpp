/**
 * @file   PixelPriorityEncoder.hpp
 * @author Simon Voigt Nesbo
 * @date   August 20, 2018
 * @brief  PixelPriorityEncoder class. Implements sorting function for std::set used
 *         by PixelDoubleColumn class, to sort pixel hits in the correct order that the
 *         priority encoder in the Alpide chip would read them out.
 */
#ifndef PIXEL_PRIORITY_ENCODER_HPP
#define PIXEL_PRIORITY_ENCODER_HPP

#include <memory>
#include "PixelHit.hpp"

/**
   @brief Comparator class/function for use with the PixelHit class in the
   std::set container, which allows the container to sort the PixelHit
   entries in a meaningful way. The picture below is from the ALPIDE operations
   manual, and shows:
   - Left: 512 rows x 1024 columns of pixels, divided into 32 regions
   - Middle: 32 columns (16 double columns) x 512 rows in a region
   - Right: Index/numbering/address of pixels within a double column, and the
   priority encoder between the columns. The priority encoder starts with the pixel
   that has the lowest address, and prioritizes them in increasing order.

   The idea is that only pixels that have hits will be stored in the std::set, and the pixels
   in the set are read out in increasing order (starting with index 0), and this class implements
   the actual prioritization of the pixels in the set.
   @image html regions-columns-indexes.png
   @image latex regions-columns-indexes.png "Overview of how regions and columns are indexed, and how pixels are indexed in double columns, in the Alpide chip."
*/
class PixelPriorityEncoder
{
public:
  /**
     @brief Overloaded () function, allows the std::set to use this function to compare two
     PixelHit classes in the set, and determine which of them should come first when sorting them.
     The prioritization works like this:
     - Lower rows prioritized first
     - For even rows, the column 0 pixel comes first
     - For odd rows, the column 1 pixel comes first
     @param leftIn Left side argument
     @param rightIn Right side argument
     @return True if leftIn has highest priority, false if rightIn has higest priority
  */
  bool operator()(const std::shared_ptr<PixelHit> &leftIn,
                  const std::shared_ptr<PixelHit> &rightIn)
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


#endif
