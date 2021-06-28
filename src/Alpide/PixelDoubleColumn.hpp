/**
 * @file   PixelDoubleColumn.hpp
 * @author Simon Voigt Nesbo
 * @date   August 31, 2018
 * @brief  PixelDoubleColumn class
 *
 */


///@addtogroup pixel_stuff
///@{
#ifndef PIXEL_COL_H
#define PIXEL_COL_H

#include "alpide_constants.hpp"
#include "PixelPriorityEncoder.hpp"
#include <set>
#include <memory>
#include <cstdint>


class PixelDoubleColumn
{
private:
  std::set<std::shared_ptr<PixelHit>, PixelPriorityEncoder> pixelColumn;
public:
  bool setPixel(unsigned int col_num, unsigned int row_num);
  bool setPixel(const std::shared_ptr<PixelHit> &pixel);
  void clear(void);
  bool inspectPixel(unsigned int col_num, unsigned int row_num);
  std::shared_ptr<PixelHit> readPixel(void);
  unsigned int pixelHitsRemaining(void);
};


#endif
///@}
