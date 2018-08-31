/**
 * @file   pixel_matrix.h
 * @author Simon Voigt Nesbo
 * @date   December 11, 2016
 * @brief  Header file for pixel matrix class.
 *
 *         Pixel matrix class comprises all the pixel regions, which allows to
 *         interface in terms of absolute coordinates with the pixel matrix.
 *         Special version for the Alpide Dataflow SystemC model.
 *
 */


///@defgroup pixel_stuff Pixel Matrix, Columns, and Priority Encoder
///@ingroup alpide
///@{
#ifndef PIXEL_MATRIX_H
#define PIXEL_MATRIX_H

#include "PixelDoubleColumn.hpp"
#include <vector>
#include <queue>
#include <list>
#include <map>
#include <memory>
#include <cstdint>


class PixelMatrix
{
private:
  ///@brief mColumnBuffs holds multi event buffers of pixel columns
  ///       The queue represent the MEBs, and the vector the pixel columns.
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

  ///@brief Number of pixel hits that were actually latched into an MEB
  std::uint64_t mLatchedPixelHitCount = 0;

  ///@brief Number of pixel hits that could not be latched into an MEB because
  ///       it already existed in the MEB (ie. duplicate hits from more than one
  ///       interaction event)
  std::uint64_t mDuplicatePixelHitCount = 0;

protected:
  ///@todo Several of these functions will be exposed "publically" to users of
  ///      the Alpide class.. most of them should be made private, or maybe use
  ///      protected inheritance in Alpide class? But the user should still
  ///      have access to setPixel()..
public:
  virtual void newEvent(uint64_t event_time);
  void deleteEvent(uint64_t event_time);
  void flushOldestEvent(void);
  void setPixel(unsigned int col, unsigned int row);
  void setPixel(const std::shared_ptr<PixelHit> &pixel);
  bool regionEmpty(int start_double_col, int stop_double_col);
  bool regionEmpty(int region);
  std::shared_ptr<PixelHit> readPixel(uint64_t time_now,
                                      int start_double_col = 0,
                                      int stop_double_col = N_PIXEL_COLS/2);
  std::shared_ptr<PixelHit> readPixelRegion(int region, uint64_t time_now);
  int getNumEvents(void) {return mColumnBuffs.size();}
  int getHitsRemainingInOldestEvent(void);
  int getHitTotalAllEvents(void);
  std::map<unsigned int, std::uint64_t> getMEBHisto(void) const {
    return mMEBHistogram;
  }
  std::uint64_t getLatchedPixelHitCount(void) const {return mLatchedPixelHitCount;}
  std::uint64_t getDuplicatePixelHitCount(void) const {return mDuplicatePixelHitCount;}
};


#endif
///@}
