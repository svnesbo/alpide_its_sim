/**
 * @file   EventDigits.hpp
 * @author Simon Voigt Nesbo
 * @date   March 5, 2019
 * @brief  Class that holds "digits" (ie. pixel hits, or particle hits) for an event.
 */

#ifndef EVENT_DIGITS_H
#define EVENT_DIGITS_H

#include <vector>
#include "Alpide/PixelHit.hpp"

class EventDigits {
  std::vector<PixelHit> mHitDigits;

public:
  void addHit(int col, int row, unsigned int chip_id) {
    mHitDigits.emplace_back(col, row, chip_id);
  }

  auto getDigitsIterator(void) const -> std::vector<PixelHit>::const_iterator
  {
    return mHitDigits.begin();
  }

  auto getDigitsEndIterator(void) const -> std::vector<PixelHit>::const_iterator
  {
    return mHitDigits.end();
  }

  size_t size(void) const {return mHitDigits.size();}

  void printEvent(void) const {
    for(auto it = mHitDigits.begin(); it != mHitDigits.end(); it++) {
      std::cout << "Chip  " << it->getChipId() << "  ";
      std::cout << it->getCol() << ":";
      std::cout << it->getRow() << std::endl;
    }
  }
};


#endif /* EVENT_DIGITS_H */
