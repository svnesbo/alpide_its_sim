/**
 * @file   pixel_matrix.cpp
 * @Author Simon Voigt Nesbo
 * @date   November 28, 2016
 * @brief  Pixel matrix class comprising all the pixel regions, which allows to
 *         interface in terms of absolute coordinates with the pixel matrix.
 *
 * Detailed description of file.
 */

#include <iostream>
#include "pixel_matrix.h"

PixelMatrix::PixelMatrix()
{

}


PixelMatrix::setPixel(unsigned int col, unsigned int row)
{
  if(row_num >= N_PIXEL_ROWS) {
    std::cout << "Error. Pixel row address > number of rows. Hit ignored." << std::endl;
  } else if(col_num >= N_PIXEL_COLS) {
    std::cout << "Error. Pixel row address > number of cols. Hit ignored." << std::endl;
  } else {
    unsigned int region_num = col / N_PIXELS_PER_REGION;
    unsigned int region_col_num = col % N_PIXELS_PER_REGION;
    
    mRegions[region_num].setPixel(region_col_num, row);
  }  
}


bool PixelMatrix::getPixel(unsigned int col, unsigned int row)
{
  if(row_num >= N_PIXEL_ROWS) {
    std::cout << "Error. Pixel row address > number of rows. Hit ignored." << std::endl;

    //@todo Maybe implement some exceptions or something if we are out of bounds here?    
    return false;
  } else if(col_num >= N_PIXEL_COLS) {
    std::cout << "Error. Pixel row address > number of cols. Hit ignored." << std::endl;

    //@todo Maybe implement some exceptions or something if we are out of bounds here?
    return false; 
  } else {
    unsigned int region_num = col / N_PIXELS_PER_REGION;
    unsigned int region_col_num = col % N_PIXELS_PER_REGION;
    
    return mRegions[region_num].getPixel(region_col_num, row);
  }
}


PixelRegion* PixelMatrix::getRegion(unsigned int region_num)
{
  if(region_num > N_REGIONS) {
    return NULL;
  }

  return &this->mRegions[region_num];
}
