/**
 * @file   pixel_region.cpp
 * @Author Simon Voigt Nesbo
 * @date   November 27, 2016
 * @brief  Class declaration for pixel regions
 *
 * Detailed description of file.
 */


#include "pixel_region.h"


void PixelRegion::setPixel(unsigned int col_num, unsigned int row_num) {
  if(row_num < 0 || row_num >= N_PIXEL_ROWS) {
    throw std::out_of_range ("row_num");
  } else if(col_num < 0 || col_num >= N_PIXEL_COLS_PER_REGION) {
    throw std::out_of_range ("col_num");
  } else {
    // Set the pixel in the right row and column, taking into account that
    // we need to address the right double column, and we need to address
    // the right pixel column (0 or 1) in that double column.
    dcols[col_num/2].setPixel(col_num%2);
  }
}


//@brief Check if there is a hit or not for the pixel specified by col and row,
//       without deleting the pixel from the MEB.
//@param col Column (0 or 1).
//@param row Row (0 to 511).
//@return True if there is a hit, false if not.
bool PixelRegion::inspectPixel(unsigned int col_num, unsigned int row_num)
{
  bool retval = false;

  // Out of range exception check
  if(row_num < 0 || row_num >= N_PIXEL_ROWS) {
    throw std::out_of_range ("row_num");
  } else if(col_num < 0 || col_num >= N_PIXEL_COLS_PER_REGION) {
    throw std::out_of_range ("col_num");
  }

  // Search for pixel
  else {
    retval = dcols[col_num/2].inspectPixel(col_num);
  }

  return retval;
}

///@brief Read out the next pixel from this region
//@return PixelData with hit coordinates. If no pixel hits exist, NoPixelHit is returned
//        (PixelData object with coords = (-1,-1)).  
PixelData PixelRegion::readPixel(void) {
  PixelData pixelHit = NoPixelHit; // Defaults to no hits
    
  // Search for a double column that has more pixels to read out, starting at double column 0
  for(int i = 0; i < N_PIXEL_DOUBLE_COLS_PER_REGION; i++) {
    // Return next prioritized hit for this double column if it has more pixels to read out
    if(dcols[i].pixelHitsRemaining() > 0) {
      pixelHit = dcols[i].readPixel();

      // Update column coords to take position in region into account
      int col = pixelHit.getCol();
      col += i*2;
      pixelHit.setCol(col);
    }
  }

  return pixelHit;
}
  
///@brief Returns how many pixel hits (in all double columns) that have not been read out from the MEBs yet
unsigned int PixelRegion::pixelHitsRemaining(void) {
  int num_hits = 0;
  for(int i = 0; i < N_PIXEL_DOUBLE_COLS_PER_REGION; i++)
    num_hits += dcols[i].pixelHitsRemaining();

  return num_hits;
}
