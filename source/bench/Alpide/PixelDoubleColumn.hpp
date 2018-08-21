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
#include "PixelPriorityEncoder.hpp"
#include <set>
#include <memory>


class PixelDoubleColumn
{
private:
  std::set<std::shared_ptr<PixelHit>, PixelPriorityEncoder> pixelColumn;
public:
  void setPixel(unsigned int col_num, unsigned int row_num);
  void setPixel(const std::shared_ptr<PixelHit> &pixel);
  void clear(void);
  bool inspectPixel(unsigned int col_num, unsigned int row_num);
  std::shared_ptr<PixelHit> readPixel(void);
  unsigned int pixelHitsRemaining(void);
};


#endif
///@}
