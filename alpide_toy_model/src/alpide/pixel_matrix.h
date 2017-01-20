/**
 * @file   pixel_matrix.h
 * @author Simon Voigt Nesbo
 * @date   December 11, 2016
 * @brief  Header file for pixel matrix class.
 *
 *         Pixel matrix class comprises all the pixel regions, which allows to
 *         interface in terms of absolute coordinates with the pixel matrix.
 *         Special version for the Alpide toy model with no region readout.
 */

#ifndef PIXEL_MATRIX_H
#define PIXEL_MATRIX_H

#include "pixel_col.h"
#include <vector>
#include <queue>
#include <list>
#include <map>
#include <cstdint>


class PixelMatrix
{
private:
  ///@brief mColumnBuffs holds multi event buffers of pixel columns
  ///       The queue represent the MEBs, and the vector the pixel columns.
  ///       For the "toy model" the size of the queue is not limited to the 3
  ///       MEBs found in the Alpide, we allow it to grow and will plot it's size
  ///       over time in a histogram. The probabiity of of having the size > 3
  ///       will essentially be a measure of BUSY in the MEBs.
  ///@todo  Implement event ID somewhere. Maybe make an MEB class, and use it as
  ///       the datatype for this queue?
  std::queue< std::vector<PixelDoubleColumn> > mColumnBuffs;

  ///@brief Each entry here corresponds to one entry in mColumnBuffs. This variable
  ///       keeps track of the number of pixel left in the columns in each entry in
  ///       mColumnBuffs.
  std::list<int> mColumnBuffsPixelsLeft;

  ///@brief This map contains histogram values over MEB usage. The key is the number
  ///       of MEBs in use, and the value is the total time duration for that key.
  std::map<unsigned int, std::uint64_t> mMEBHistogram;

  ///@brief Last time the MEB histogram was updated
  uint64_t mMEBHistoLastUpdateTime = 0;

  ///@brief Number of (trigger) events that are accepted into an MEB by the chip
  uint64_t mTriggerEventsAccepted = 0;

  ///@brief Triggered mode: If 3 MEBs are already full, the chip will not accept more events
  ///                       until one of those 3 MEBs have been read out. This variable is counted
  ///                       up for each event that is not accepted.
  ///       Continuous mode: The Alpide chip will always guarantee that there is a free MEB slice
  ///                        in continuous mode. It does this by deleting the oldest MEB slice (even
  ///                        if it has not been read out) when the 3rd one is filled. This variable
  ///                        also counts up in that case.
  uint64_t mTriggerEventsRejected = 0;

  ///@brief True: Continuous, False: Triggered
  bool mContinuousMode;
  
public:
  PixelMatrix(bool continuous_mode);
  bool newEvent(uint64_t event_time);
  void setPixel(unsigned int col, unsigned int row);
  PixelData readPixel(uint64_t event_time,
                      int start_double_col = 0,
                      int stop_double_col = N_PIXEL_COLS/2);
  PixelData readPixelRegion(int region, uint64_t event_time);
  int getNumEvents(void) {return mColumnBuffs.size();}
  int getHitsRemainingInOldestEvent(void);
  int getHitTotalAllEvents(void);
  uint64_t getTriggerEventsAcceptedCount(void) const {return mTriggerEventsAccepted;}
  uint64_t getTriggerEventsRejectedCount(void) const {return mTriggerEventsRejected;}
  std::map<unsigned int, std::uint64_t> getMEBHisto(void) const {
    return mMEBHistogram;
  }
};


#endif
