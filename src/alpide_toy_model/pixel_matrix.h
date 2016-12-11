/**
 * @file   pixel_matrix.h
 * @Author Simon Voigt Nesbo
 * @date   December 11, 2016
 * @brief  Pixel matrix class comprising all the pixel regions, which allows to
 *         interface in terms of absolute coordinates with the pixel matrix.
 *         Special version for the Alpide toy model with no region readout.
 *
 * Detailed description of file.
 */

#ifndef PIXEL_MATRIX_H
#define PIXEL_MATRIX_H

#include "pixel_col.h"
#include <vector>
#include <queue>


class PixelMatrix
{
private:
  //@brief mColumnBuffs holds multi event buffers of pixel columns
  //       The queue represent the MEBs, and the vector the pixel columns.
  //       For the "toy model" the size of the queue is not limited to the 3
  //       MEBs found in the Alpide, we allow it to grow and will plot it's size
  //       over time in a histogram. The probabiity of of having the size > 3
  //       will essentially be a measure of BUSY in the MEBs.
  std::queue< std::vector<PixelDoubleColumn> > mColumnBuffs;

  //@brief Each entry here corresponds to one entry in mColumnBuffs. This variable
  //       keeps track of the number of pixel left in the columns in each entry in
  //       mColumnBuffs.
  std::queue<int> mColumnBuffsPixelsLeft;
  
public:
  PixelMatrix();
  void newEvent(void);
  void setPixel(unsigned int col, unsigned int row);
  PixelData readPixel(void);

  //@todo Remove? Which event should getPixel get pixels from? Doesn't really make sense..
  //bool inspectPixel(unsigned int col, unsigned int row);
  
  int getNumEvents(void) {return mColumnBuffs.size();}
  int getHitsRemainingInOldestEvent(void);
};


#endif
