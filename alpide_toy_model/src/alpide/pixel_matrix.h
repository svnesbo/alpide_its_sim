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
  
public:
  PixelMatrix();
  void newEvent(void);
  void setPixel(unsigned int col, unsigned int row);
  PixelData readPixel(int start_double_col = 0, int stop_double_col = N_PIXEL_COLS/2);
  PixelData readPixelRegion(int region);
  int getNumEvents(void) {return mColumnBuffs.size();}
  int getHitsRemainingInOldestEvent(void);
  int getHitTotalAllEvents(void);
};


#endif
