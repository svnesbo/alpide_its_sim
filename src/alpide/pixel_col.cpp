/**
 * @file   pixel_col.h
 * @Author Simon Voigt Nesbo
 * @date   November 27, 2016
 * @brief  Pixel column, double column, and priority encoder classes
 *
 * Detailed description of file.
 */

#include "pixel_col.h"

PixelDoubleColumn::setPixel(unsigned int col_num, unsigned int row_num)
{
  if(row_num >= N_PIXEL_ROWS) {
    std::cout << "Error. Pixel row address > number of rows. Hit ignored.\n";
  } else {
    // Extract last bit off column number
    unsigned int col_num_lsb = col_num & 1;

    pixelMEBColumns[strobe].insert(PixelData(col_num_lsb, row_num));
  }
}

///@brief Read out the next pixel from this double column, and erase it from the MEB.
//        Pixels are read out in an order corresponding to that of the priority encoder
//        in the Alpide chip.
//@return PixelData with hit coordinates. If no pixel hits exist, NoPixelHit is returned
//        (PixelData object with coords = (-1,-1)).
PixelData PixelDoubleColumn::readPixel(void) {
  if(pixelMEBColumns[memsel].size() == 0)
    return NoPixelHit;
    
  // Read out the next (prioritized) pixel
  PixelData pixel = *pixelMEBColumns[memsel].begin();

  // Remove the pixel when it has been read out
  pixelMEBColumns[memsel].erase(pixelMEBColumns[memsel].begin());

  return pixel;
}

PixelData PixelDoubleColumn::getPixel(unsigned int col, unsigned int row) {
  if(row_num >= N_PIXEL_ROWS) {
    std::cout << "Error. Pixel row address > number of rows. Hit ignored.\n";

    //@todo Maybe implement some exceptions or something if we are out of bounds here?    
    return NoPixelHit;
  } else {
    // Extract last bit off column number
    unsigned int col_num_lsb = col_num & 1;

    pixelMEBColumns[strobe].insert(PixelData(col_num_lsb, row_num));
  }  
}

///@brief Returns how many pixel hits (in this double column) that have not been read out from the MEBs yet
unsigned int PixelDoubleColumn::pixelsHitsRemaining(void) {
  return pixelMEBColumns[memsel].size();
}
