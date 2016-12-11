/**
 * @file   pixel_col.h
 * @Author Simon Voigt Nesbo
 * @date   November 27, 2016
 * @brief  Pixel column, double column, and priority encoder classes
 *
 * Detailed description of file.
 */

#include "pixel_col.h"
#include <stdexcept>

void PixelDoubleColumn::setPixel(unsigned int col_num, unsigned int row_num)
{
  if(row_num < 0 || row_num >= N_PIXEL_ROWS) {
    throw std::out_of_range ("row_num");
  } else if(col_num < 0 || col_num >= 2) {
    throw std::out_of_range ("col_num");
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

//@brief Check if there is a hit or not for the pixel specified by col_num and row_num,
//       without deleting the pixel from the MEB.
//@param col_num Column (0 or 1).
//@param row_num Row (0 to 511).
//@return True if there is a hit, false if not.
bool PixelDoubleColumn::inspectPixel(unsigned int col_num, unsigned int row_num) {
  bool retval = false;

  // Out of range exception check
  if(row_num < 0 || row_num >= N_PIXEL_ROWS) {
    throw std::out_of_range ("row_num");
  } else if(col_num < 0 || col_num >= 2) {
    throw std::out_of_range ("col_num");
  }

  // Search for pixel
  else{
    PixelData pixel = PixelData(col_num, row_num);
    if(pixelMEBColumns[strobe].find(pixel) != pixelMEBColumns[strobe].end()) {
      // Only actual hits are stored in a set, so if the coords are found in the
      // set it always means we have a hit.
      retval = true;
    }
  }

  return retval;
}


///@brief Returns how many pixel hits (in this double column) that have not been read out from the MEBs yet
unsigned int PixelDoubleColumn::pixelHitsRemaining(void) {
  return pixelMEBColumns[memsel].size();
}
