/**
 * @file   pixel_matrix.h
 * @Author Simon Voigt Nesbo
 * @date   November 28, 2016
 * @brief  Pixel matrix class comprising all the pixel regions, which allows to
 *         interface in terms of absolute coordinates with the pixel matrix.
 *
 * Detailed description of file.
 */

#ifndef PIXEL_MATRIX_H
#define PIXEL_MATRIX_H

#include "pixel_region.h"


class PixelMatrix
{
private:
  PixelRegion mRegions[N_REGIONS];
public:
  PixelMatrix();
  void setPixel(unsigned int col, unsigned int row);
  bool getPixel(unsigned int col, unsigned int row);
  PixelRegion* getRegion(unsigned int region_num);
};


#endif
