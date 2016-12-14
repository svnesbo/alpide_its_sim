/**
 * @file   pixel_matrix.cpp
 * @Author Simon Voigt Nesbo
 * @date   December 11, 2016
 * @brief  Pixel matrix class comprising all the pixel regions, which allows to
 *         interface in terms of absolute coordinates with the pixel matrix.
 *         Special version for the Alpide toy model with no region readout.
 *
 * Detailed description of file.
 */

#include <iostream>
#include "pixel_matrix.h"

PixelMatrix::PixelMatrix()
{

}


//@brief Indicate that we are starting a new event. The next calls to setPixel
//       will add pixels to the new event.
void PixelMatrix::newEvent(void)
{
  mColumnBuffs.push(std::vector<PixelDoubleColumn>(N_PIXEL_COLS/2));

  std::cout << "Pushed new PixelDoubleColumn vector to mColumnBuffs." << std::endl;
  std::cout << "mColumnBuffs.size(): " << mColumnBuffs.size() << std::endl;
  std::cout << "mColumnBuffs.back().size(): " << mColumnBuffs.back().size() << std::endl;

  // 0 hits so far for this event
  mColumnBuffsPixelsLeft.push_back(int(0));
}


//@brief Set the pixel (ie. the pixel is hit) specified by col_num and row_num,
//       in the most recent event buffer.
//@param col_num Column (0 to N_PIXEL_COLS-1).
//@param row_num Row (0 to N_PIXEL_ROWS-1).
//@throw out_of_range If there are no events, or if col or row is outside the allowed range
void PixelMatrix::setPixel(unsigned int col, unsigned int row)
{
  // Out of range exception check
  if(getNumEvents() == 0) {
    throw std::out_of_range("No events");
  }else if(row < 0 || row >= N_PIXEL_ROWS) {
    throw std::out_of_range("row");
  } else if(col < 0 || col >= N_PIXEL_COLS) {
    throw std::out_of_range("col");
  }

  // Set the pixel
  else {
    std::vector<PixelDoubleColumn>& current_event_buffer = mColumnBuffs.back();
    int& current_event_buffer_hits_remaining = mColumnBuffsPixelsLeft.back();

    //@todo Remove
    //std::cout << "Setting pixel " << col << ":" << row << std::endl;
    //std::cout << "mColumnBuffsPixelsleft.back() before insertion: " << mColumnBuffsPixelsLeft.back() << std::endl;
    
    current_event_buffer[col/2].setPixel(col%2, row);
    current_event_buffer_hits_remaining++;

    //@todo Remove
    //std::cout << "mColumnBuffsPixelsleft.back() after insertion: " << mColumnBuffsPixelsLeft.back() << std::endl;    
  }  
}


///@brief Read out the next pixel from the pixel matrix, and erase it from the MEB.
//        This member function will read out pixels from the oldest event buffer.
//        The pixels will be by default be read out from the double columns in consecutive
//        order from 0 to (N_PIXEL_COLS/2)-1, or optionally from the double column range specified
//        by start_double_col and stop_double_col.
//        Regions are not read out in parallel with this function. But note that within a double column
//        the pixels will be read out with the order used by the priority encoder in the Alpide chip.
//@return PixelData with hit coordinates. If no pixel hits exist, NoPixelHit is returned
//        (PixelData object with coords = (-1,-1)).
//@throw  std::out_of_range if start_double_col is less than zero, or larger
//        than (N_PIXEL_COLS/2)-1.
//@throw  std::out_of_range if stop_double_col is less than one, or larger
//        than N_PIXEL_COLS/2.
//@throw  std::out_of_range if stop_double_col is greater than or equal to start_double_col
PixelData PixelMatrix::readPixel(int start_double_col, int stop_double_col) {
  PixelData pixel_retval = NoPixelHit;

  // Out of range exception check
  if(start_double_col < 0 || start_double_col > (N_PIXEL_COLS/2)-1) {
    throw std::out_of_range("start_double_col");
  } else if(stop_double_col < 1 || stop_double_col > (N_PIXEL_COLS/2)) {
    throw std::out_of_range("stop_double_col");
  } else if(start_double_col >= stop_double_col) {
    throw std::out_of_range("stop_double_col >= start_double_col");
  }
  
  // Do we have any stored events?
  if(mColumnBuffs.size() > 0) {
    std::vector<PixelDoubleColumn>& oldest_event_buffer = mColumnBuffs.front();
    int& oldest_event_buffer_hits_remaining = mColumnBuffsPixelsLeft.front();

    //@todo Remove
    //std::cout << "PixelMatrix::readPixel: oldest_event_buffer_hits_remaining = " << oldest_event_buffer_hits_remaining << std::endl;
    
    // Search for the first column that has pixels to read out
    //for(auto it = oldest_event_buffer.begin(); it != oldest_event_buffer.end(); it++) {
    for(int i = start_double_col; i < stop_double_col; i++) {
      if(oldest_event_buffer[i].pixelHitsRemaining() > 0) {
        pixel_retval = oldest_event_buffer[i].readPixel();

        // pixel_retval.mCol is either 0 or 1 (values in double column), correct to take the
        // double column number into account
        pixel_retval.setCol(2*i + pixel_retval.getCol());
      
        oldest_event_buffer_hits_remaining--;
        break;
      }
    }

    // If this was the last hit in this event buffer, remove the event buffer from the queue  
    if(oldest_event_buffer_hits_remaining == 0) {
      std::cout << "Popping oldest MEB buffer\n";
      mColumnBuffs.pop();
      mColumnBuffsPixelsLeft.pop_front();
    }      
  }



  return pixel_retval;
}


///@brief Read out the next pixel from the specified region in the pixel matrix, and erase it
//        from the MEB. This member function will read out pixels from the oldest event buffer.
//        The pixels in the desired region will be read out from the double columns in consecutive
//        order from 0 to N_PIXEL_DOUBLE_COLS_PER_REGION-1.
//        Note that within a double column the pixels will be read out with the order
//        used by the priority encoder in the Alpide chip.
//@param  region The region number to read out a pixel from
//@return PixelData with hit coordinates. If no pixel hits exist, NoPixelHit is returned
//        (PixelData object with coords = (-1,-1)).
//@throw  std::out_of_range if region is less than zero, or greater than N_REGIONS-1
PixelData PixelMatrix::readPixelRegion(int region) {
  if(region < 0 || region >= N_REGIONS)
    throw std::out_of_range("region");

  int start_double_col = N_PIXEL_DOUBLE_COLS_PER_REGION*region;
  //int stop_double_col = (N_PIXEL_DOUBLE_COLS_PER_REGION*(region+1))-1;
  int stop_double_col = (N_PIXEL_DOUBLE_COLS_PER_REGION*(region+1));

  //@todo Remove
  //std::cout << "start_double_col: " << start_double_col << std::endl;
  //std::cout << "stop_double_col: " << stop_double_col << std::endl;  

  return readPixel(start_double_col, stop_double_col);
}


//@brief Return the number of hits in the oldest of the events stored in multi event buffers
//@return Number of hits in oldest event. If there are no events left, return zero.
int PixelMatrix::getHitsRemainingInOldestEvent(void)
{
  if(getNumEvents() == 0) {
    //std::cout << "getHitsRemainingInOldestEvent(): No events, returning zero." << std::endl;
    return 0;
  }
  else {
    int& oldest_event_buffer_hits_remaining = mColumnBuffsPixelsLeft.front();    
    //std::cout << "getHitsRemainingInOldestEvent(): " << getNumEvents() << " events, " << oldest_event_buffer_hits_remaining << " pixels at .front()" << std::endl;
    return mColumnBuffsPixelsLeft.front();
  }
}


int PixelMatrix::getHitTotalAllEvents(void)
{
  int hit_sum = 0;
  
  if(getNumEvents() > 0) {
    for(std::list<int>::iterator it = mColumnBuffsPixelsLeft.begin(); it != mColumnBuffsPixelsLeft.end(); it++) {
      //@todo Remove 
      //std::cout << "Hits in event: " << *it << std::endl;
      
      hit_sum += *it;
    }
  }

  return hit_sum;
}


//@todo Remove? Which event should this function get the pixels from? Doesn't really make sense..
//
//@brief Check if there is a hit or not for the pixel specified by col_num and row_num,
//       without deleting the pixel from the MEB.
//@param col_num Column (0 or N_PIXEL_COLS-1).
//@param row_num Row (0 to N_PIXEL_ROWS-1).
//@return True if there is a hit, false if not.
/*
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
*/