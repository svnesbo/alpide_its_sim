/**
 * @file   pixel_region.cpp
 * @Author Simon Voigt Nesbo
 * @date   November 27, 2016
 * @brief  Class declaration for pixel regions
 *
 * Detailed description of file.
 */


#include "pixel_region.h"


PixelRegion::setPixel(unsigned int col_num, unsigned int row_num) {
  if(row_num >= N_PIXEL_ROWS) {
    std::cout << "Error. Pixel row address > number of rows. Hit ignored.\n";
  } else if(col_num >= N_PIXEL_COLS_PER_REGION) {
    std::cout << "Error. Pixel row address > number of cols. Hit ignored.\n";
  } else {
    dcols[col_num/2].setPixel(col_num);
  }
}


bool PixelRegion::getPixel(unsigned int col, unsigned int row)
{
  if(row_num >= N_PIXEL_ROWS) {
    std::cout << "Error. Pixel row address > number of rows. Hit ignored.\n";

    //@todo Maybe implement some exceptions or something if we are out of bounds here?    
    return false;
  } else if(col_num >= N_PIXEL_COLS_PER_REGION) {
    std::cout << "Error. Pixel row address > number of cols. Hit ignored.\n";

    //@todo Maybe implement some exceptions or something if we are out of bounds here?
    return false; 
  } else {
    return dcols[col_num/2].getPixel(col_num);
  }
}

///@brief Read out the next pixel from this region
//@return PixelData with hit coordinates. If no pixel hits exist, NoPixelHit is returned
//        (PixelData object with coords = (-1,-1)).  
PixelData PixelRegion::readPixel(void) {
  PixelData pixelHit = NoPixelHit; // Defaults to no hits
    
  // Search for a double column that has more pixels to read out, starting at double column 0
  for(int i = 0; i < N_PIXEL_COLS_PER_REGION/2; i++) {
    // Return next prioritized hit for this double column if it has more pixels to read out
    if(dcols[i].pixelHitsRemaining() > 0) {
      pixelHit = dcols[i].readPixel();

      // Update column coords to take position in region into account
      pixelHit.col += i*2;
    }
  }

  return pixelHit;
}
  
///@brief Returns how many pixel hits (in all double columns) that have not been read out from the MEBs yet
unsigned int PixelRegion::pixelHitsRemaining(void) {
  int num_hits = 0;
  for(int i = 0; i < N_PIXELS_PER_REGION/2; i++)
    num_hits += dcols[i].pixelHitsRemaining();

  return num_hits;
}
