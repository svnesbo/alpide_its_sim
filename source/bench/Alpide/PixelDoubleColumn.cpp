/**
 * @file   pixel_col.h
 * @author Simon Voigt Nesbo
 * @date   November 27, 2016
 * @brief  Source file for pixel column, double column, and priority encoder classes
 */

#include "PixelDoubleColumn.hpp"
#include <stdexcept>
#include <iostream>


///@brief Constructor for pixel data based on region number, priority encoder number in region,
///       and pixel address in priority encoder. This is how the data are specified when they
///       are transmitted as data long/short words.
///@param[in] region Region number
///@param[in] pri_enc Priority encoder number in region (ie. double column number in region).
///@param[in] addr Prioritized address in priority encoder
PixelData::PixelData(int region, int pri_enc, int addr,
                     std::shared_ptr<PixelReadoutStats> pix_stats)
  : mPixelReadoutStats(pix_stats)
{
  mRow = addr >> 1;
  mCol = ((addr&1) ^ (mRow&1)); // LSB of column
  mCol = (region << 5) | (pri_enc << 1) | mCol;
}


bool PixelData::operator==(const PixelData& rhs) const
{
  return (this->mCol == rhs.mCol) && (this->mRow == rhs.mRow);
}


bool PixelData::operator>(const PixelData& rhs) const
{
  bool retval = false;

  if(mCol < rhs.mCol)
    retval = false;
  else if(mCol > rhs.mCol)
    retval = true;
  else {
    if(mRow < rhs.mRow)
      retval = false;
    else if(mRow > rhs.mRow)
      retval = true;
    else
      retval = false;
  }

  return retval;
}

bool PixelData::operator<(const PixelData& rhs) const
{
  return rhs > *this;
}

bool PixelData::operator>=(const PixelData& rhs) const
{
  return !(*this < rhs);
}

bool PixelData::operator<=(const PixelData& rhs) const
{
  return !(*this > rhs);
}


///@brief Set a pixel in a pixel double column object.
///@param[in] col_num column number of pixel, must be 0 or 1.
///@param[in] row_num row number of pixel, must be in the range 0 to N_PIXEL_ROWS-1
void PixelDoubleColumn::setPixel(unsigned int col_num, unsigned int row_num)
{
  pixelColumn.insert(std::make_shared<PixelData>(col_num, row_num));
}


///@brief Set a pixel in a pixel double column object.
///@param[in] pixel shared pointer to PixelData object.
void PixelDoubleColumn::setPixel(const std::shared_ptr<PixelData> &pixel)
{
  pixelColumn.insert(pixel);
}

///@brief Clear (flush) contents of double column
void PixelDoubleColumn::clear(void) {
  pixelColumn.clear();
}


///@brief Read out the next pixel from this double column, and erase it from the MEB.
///       Pixels are read out in an order corresponding to that of the priority encoder
///       in the Alpide chip.
///@return shared_ptr to PixelData with hit coordinates. If no pixel hits exist, a shared_ptr
///       to NoPixelHit is returned (PixelData object with coords = (-1,-1)).
std::shared_ptr<PixelData> PixelDoubleColumn::readPixel(void) {
  if(pixelColumn.size() == 0)
    return std::make_shared<PixelData>(NoPixelHit);

  // Read out the next (prioritized) pixel
  std::shared_ptr<PixelData> pixel = *pixelColumn.begin();

  // Remove the pixel when it has been read out
  pixelColumn.erase(pixelColumn.begin());

  return pixel;
}

///@brief Check if there is a hit or not for the pixel specified by col_num and row_num,
///       without deleting the pixel from the MEB.
///@param[in] col_num column number of pixel, must be 0 or 1.
///@param[in] row_num row number of pixel, must be in the range 0 to N_PIXEL_ROWS-1
///@return True if there is a hit, false if not.
///@throws std::out_of_range if col_num or row_num is not in the specified range.
bool PixelDoubleColumn::inspectPixel(unsigned int col_num, unsigned int row_num) {
#ifdef EXCEPTION_CHECKS
  // Out of range exception check
  if(row_num >= N_PIXEL_ROWS) {
    throw std::out_of_range ("row_num");
  } else if(col_num >= 2) {
    throw std::out_of_range ("col_num");
  }
#endif

  // Search for pixel
  for(auto pix_it = pixelColumn.begin(); pix_it != pixelColumn.end(); pix_it++) {
    if(*pix_it->getCol() == col_num && *pix_it->getRow() == row_num)
      return true;
  }

  return false;
}


///@brief Returns how many pixel hits (in this double column) that have not been read out from the MEBs yet
unsigned int PixelDoubleColumn::pixelHitsRemaining(void) {
  return pixelColumn.size();
}
