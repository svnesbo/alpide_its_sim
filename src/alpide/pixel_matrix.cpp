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


//@brief Set the pixel (ie. the pixel is hit) specified by col_num and row_num.
//@param col_num Column (0 to N_PIXEL_COLS-1).
//@param row_num Row (0 to N_PIXEL_ROWS-1).
void PixelMatrix::setPixel(unsigned int col, unsigned int row)
{
  // Out of range exception check
  if(row_num < 0 || row_num >= N_PIXEL_ROWS) {
    throw std::out_of_range ("row_num");
  } else if(col_num < 0 || col_num >= N_PIXEL_COLS) {
    throw std::out_of_range ("col_num");
  }

  // Set the pixel
  else {
    unsigned int region_num = col / N_PIXELS_PER_REGION;
    unsigned int region_col_num = col % N_PIXELS_PER_REGION;
    
    mRegions[region_num].setPixel(region_col_num, row);
  }  
}


// @todo Remove this. I don't want readout for the pixel matrix class, that's only for the toy model.
//
// ///@brief Read out the next pixel from the pixel matrix, and erase it from the MEB.
// //        This member function will read out columns in consecutive order from
// //        0 to N_PIXEL_COLS-1. Regions are not read out in parallel with this function.
// //        But note that for a column the pixels will be read out with the order
// //        used by the priority encoder in the Alpide chip.
// //@return PixelData with hit coordinates. If no pixel hits exist, NoPixelHit is returned
// //        (PixelData object with coords = (-1,-1)).
// PixelData PixelDoubleColumn::readPixel(void) {
//   PixelData pixel_retval = NoPixelHit;

//   // Search for the first column, for the current event (memsel), that has pixels to read out
//   for(auto it = mColumns[memsel].begin(); it != mColumns[memsel].end(); it++) {
//     if(it->pixelHitsRemaining() > 0) {
//       // Read out pixel and break from loop
//       pixel_retval = it->readPixel();
//       break;
//     }
//   }

//   return pixel_retval;
// }


//@brief Check if there is a hit or not for the pixel specified by col_num and row_num,
//       without deleting the pixel from the MEB.
//@param col_num Column (0 or N_PIXEL_COLS-1).
//@param row_num Row (0 to N_PIXEL_ROWS-1).
//@return True if there is a hit, false if not.
bool PixelMatrix::inspectPixel(unsigned int col, unsigned int row)
{
  // Out of range exception check
  if(row_num < 0 || row_num >= N_PIXEL_ROWS) {
    throw std::out_of_range ("row_num");
  } else if(col_num < 0 || col_num >= N_PIXEL_COLS) {
    throw std::out_of_range ("col_num");
  } else { // Get the pixel
    unsigned int region_num = col / N_PIXELS_PER_REGION;
    unsigned int region_col_num = col % N_PIXELS_PER_REGION;
    
    return mRegions[region_num].getPixel(region_col_num, row);
  }
}

//@brief Get a pointer to a region 
//@param region_num The region number
//@return Pointer to desired region
PixelRegion* PixelMatrix::getRegion(unsigned int region_num)
{
  // Out of range exception check
  if(region_num > N_REGIONS) {
    throw std::out_of_range ("region_num");
  }

  return &this->mRegions[region_num];
}
