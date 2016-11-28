/**
 * @file   pixel_region.h
 * @Author Simon Voigt Nesbo
 * @date   November 27, 2016
 * @brief  Class declaration for pixel regions
 *
 * Detailed description of file.
 */

#ifndef PIXEL_REGION_H
#define PIXEL_REGION_H

#include "pixel_col.h"


class PixelRegion
{
private:
  PixelDoubleColumn dcols[N_PIXEL_COLS_PER_REGION/2];
public:
  setPixel(unsigned int col_num, unsigned int row_num);
  bool getPixel(unsigned int col_num, unsigned int row_num);
  PixelData readPixel(void);
  unsigned int pixelHitsRemaining(void);
};


#endif
