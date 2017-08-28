/**
 * @file   pixel_matrix.cpp
 * @author Simon Voigt Nesbo
 * @date   December 11, 2016
 * @brief  Source file for pixel matrix class.
 *
 *         Pixel matrix class comprises all the pixel regions, which allows to
 *         interface in terms of absolute coordinates with the pixel matrix.
 *         Special version for the Alpide Dataflow SystemC model.
 */

#include <iostream>
#include "PixelMatrix.hpp"


///@brief Indicate to the Alpide that we are starting on a new event. If the call is
///       successful a new MEB slice is created, and the next calls to setPixel will add pixels
///       to the new event.
///@param[in] event_time Simulation time when the event is pushed/latched into MEB
///                  (use current simulation time).
void PixelMatrix::newEvent(uint64_t event_time)
{
  // Update the histogram value for the previous MEB size, with the duration
  // that has passed since the last update, before pushing this event to the MEBs
  unsigned int MEB_size = mColumnBuffs.size();
  mMEBHistogram[MEB_size] += event_time - mMEBHistoLastUpdateTime;
  mMEBHistoLastUpdateTime = event_time;

  mColumnBuffs.push(std::vector<PixelDoubleColumn>(N_PIXEL_COLS/2));
  mColumnBuffsPixelsLeft.push_back(int(0)); // 0 hits so far for this event
}


///@brief Flush the oldest event by clearing all double columns and setting its size to zero
void PixelMatrix::flushOldestEvent(void)
{
  if(mColumnBuffs.empty() == false) {
    std::vector<PixelDoubleColumn>& oldest_event_buffer = mColumnBuffs.front();
    int& oldest_event_buffer_hits_remaining = mColumnBuffsPixelsLeft.front();

    for(auto it = oldest_event_buffer.begin(); it != oldest_event_buffer.end(); it++) {
      it->clear();
    }

    oldest_event_buffer_hits_remaining = 0;
  }
}


///@brief Delete the oldest event from the MEB (if there are any events at all,
///       calling this function with no events is fine).
///@param[in] time_now Simulation time when this readout is occuring
///@throw out_of_range If there are no events, or if col or row is outside the allowed range
void PixelMatrix::deleteEvent(uint64_t time_now)
{
  if(getNumEvents() > 0) {
    // Update the histogram value for the previous MEB size, with the duration
    // that has passed since the last update, before popping this MEB.
    unsigned int MEB_size = mColumnBuffs.size();
    mMEBHistogram[MEB_size] += time_now - mMEBHistoLastUpdateTime;
    mMEBHistoLastUpdateTime = time_now;

    mColumnBuffsPixelsLeft.pop_front();
    mColumnBuffs.pop();
  }
#ifdef EXCEPTION_CHECKS
  // Out of range exception check
  else {
    throw std::out_of_range("No events");
  }
#endif
}


///@brief Set the pixel (ie. the pixel is hit) specified by col_num and row_num,
///       in the most recent event buffer.
///@param[in] col Column (0 to N_PIXEL_COLS-1).
///@param[in] row Row (0 to N_PIXEL_ROWS-1).
///@throw out_of_range If there are no events, or if col or row is outside the allowed range
void PixelMatrix::setPixel(unsigned int col, unsigned int row)
{
#ifdef EXCEPTION_CHECKS
  // Out of range exception check
  if(mColumnBuffs.empty() == true) {
    throw std::out_of_range("No events");
  }else if(row >= N_PIXEL_ROWS) {
    throw std::out_of_range("row");
  } else if(col >= N_PIXEL_COLS) {
    throw std::out_of_range("col");
  }
#endif

  // Set the pixel
  std::vector<PixelDoubleColumn>& current_event_buffer = mColumnBuffs.back();
  int& current_event_buffer_hits_remaining = mColumnBuffsPixelsLeft.back();

  current_event_buffer[col/2].setPixel(col%2, row);
  current_event_buffer_hits_remaining++;
}


///@brief  Check if the region denoted by start_double_col and stop_double_col is empty.
///@param[in]  start_double_col Start of region in terms of double columns
///@param[in]  stop_double_col End of region in terms of double columns
///@return True if empty
///@throw  std::out_of_range if start_double_col is less than zero, or larger
///        than (N_PIXEL_COLS/2)-1.
///@throw  std::out_of_range if stop_double_col is less than one, or larger
///        than N_PIXEL_COLS/2.
///@throw  std::out_of_range if stop_double_col is greater than or equal to start_double_col
bool PixelMatrix::regionEmpty(int start_double_col, int stop_double_col) {
  bool region_empty = true;

#ifdef EXCEPTION_CHECKS
  // Out of range exception check
  if(start_double_col < 0 || start_double_col > (N_PIXEL_COLS/2)-1) {
    throw std::out_of_range("start_double_col");
  } else if(stop_double_col < 1 || stop_double_col > (N_PIXEL_COLS/2)) {
    throw std::out_of_range("stop_double_col");
  } else if(start_double_col >= stop_double_col) {
    throw std::out_of_range("stop_double_col >= start_double_col");
  }
#endif

  // Do we have any stored events?
  if(mColumnBuffs.empty() == false) {
    std::vector<PixelDoubleColumn>& oldest_event_buffer = mColumnBuffs.front();

    // Search for the first column that has pixels
    for(int i = start_double_col; i < stop_double_col; i++) {
      if(oldest_event_buffer[i].pixelHitsRemaining() > 0) {
        region_empty = false;
        break;
      }
    }
  }

  return region_empty;
}


///@brief  Check if a region of the pixel matrix is empty
///@param[in]  region The region number to check
///@return PixelData with hit coordinates. If no pixel hits exist, NoPixelHit is returned
///        (PixelData object with coords = (-1,-1)).
///@throw  std::out_of_range if region is less than zero, or greater than N_REGIONS-1
bool PixelMatrix::regionEmpty(int region) {
#ifdef EXCEPTION_CHECKS
  if(region < 0 || region >= N_REGIONS)
    throw std::out_of_range("region");
#endif

  int start_double_col = N_PIXEL_DOUBLE_COLS_PER_REGION*region;
  int stop_double_col = (N_PIXEL_DOUBLE_COLS_PER_REGION*(region+1));

  return regionEmpty(start_double_col, stop_double_col);
}


///@brief Read out the next pixel from the pixel matrix, and erase it from the MEB.
///        This member function will read out pixels from the oldest event buffer.
///        The pixels will be by default be read out from the double columns in consecutive
///        order from 0 to (N_PIXEL_COLS/2)-1, or optionally from the double column range specified
///        by start_double_col and stop_double_col.
///        Regions are not read out in parallel with this function. But note that within a double column
///        the pixels will be read out with the order used by the priority encoder in the Alpide chip.
///@param[in]  time_now Simulation time when this readout is occuring
///@param[in]  start_double_col Start double column to start searching for pixels to readout from
///@param[in]  stop_double_col Stop searching for pixels to read out when reaching this column
///@return PixelData with hit coordinates. If no pixel hits exist, NoPixelHit is returned
///        (PixelData object with coords = (-1,-1)).
///@throw  std::out_of_range if start_double_col is less than zero, or larger
///        than (N_PIXEL_COLS/2)-1.
///@throw  std::out_of_range if stop_double_col is less than one, or larger
///        than N_PIXEL_COLS/2.
///@throw  std::out_of_range if stop_double_col is greater than or equal to start_double_col
PixelData PixelMatrix::readPixel(uint64_t time_now, int start_double_col, int stop_double_col) {
  PixelData pixel_retval = NoPixelHit;

#ifdef EXCEPTION_CHECKS
  // Out of range exception check
  if(start_double_col < 0 || start_double_col > (N_PIXEL_COLS/2)-1) {
    throw std::out_of_range("start_double_col");
  } else if(stop_double_col < 1 || stop_double_col > (N_PIXEL_COLS/2)) {
    throw std::out_of_range("stop_double_col");
  } else if(start_double_col >= stop_double_col) {
    throw std::out_of_range("stop_double_col >= start_double_col");
  }
#endif

  // Do we have any stored events?
  if(mColumnBuffs.empty() == false) {
    std::vector<PixelDoubleColumn>& oldest_event_buffer = mColumnBuffs.front();
    int& oldest_event_buffer_hits_remaining = mColumnBuffsPixelsLeft.front();

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
  }

  return pixel_retval;
}


///@brief Read out the next pixel from the specified region in the pixel matrix, and erase it
///        from the MEB. This member function will read out pixels from the oldest event buffer.
///        The pixels in the desired region will be read out from the double columns in consecutive
///        order from 0 to N_PIXEL_DOUBLE_COLS_PER_REGION-1.
///        Note that within a double column the pixels will be read out with the order
///        used by the priority encoder in the Alpide chip.
///@param[in]  region The region number to read out a pixel from
///@param[in]  time_now Simulation time when this readout is occuring. Required for updating
///                   histogram data in case an MEB is done reading out.
///@return PixelData with hit coordinates. If no pixel hits exist, NoPixelHit is returned
///        (PixelData object with coords = (-1,-1)).
///@throw  std::out_of_range if region is less than zero, or greater than N_REGIONS-1
PixelData PixelMatrix::readPixelRegion(int region, uint64_t time_now) {
#ifdef EXCEPTION_CHECKS
  if(region < 0 || region >= N_REGIONS)
    throw std::out_of_range("region");
#endif

  int start_double_col = N_PIXEL_DOUBLE_COLS_PER_REGION*region;
  int stop_double_col = (N_PIXEL_DOUBLE_COLS_PER_REGION*(region+1));

  return readPixel(time_now, start_double_col, stop_double_col);
}


///@brief Return the number of hits in the oldest of the events stored in multi event buffers
///@return Number of hits in oldest event. If there are no events left, return zero.
int PixelMatrix::getHitsRemainingInOldestEvent(void)
{
  if(mColumnBuffs.empty() == true) {
    return 0;
  }
  else {
    return mColumnBuffsPixelsLeft.front();
  }
}


///@brief Get total number of hits in all Multi Event Buffers.
///@return Total number of hits.
int PixelMatrix::getHitTotalAllEvents(void)
{
  int hit_sum = 0;

  if(mColumnBuffs.empty() == false) {
    for(std::list<int>::iterator it = mColumnBuffsPixelsLeft.begin(); it != mColumnBuffsPixelsLeft.end(); it++) {
      hit_sum += *it;
    }
  }

  return hit_sum;
}
