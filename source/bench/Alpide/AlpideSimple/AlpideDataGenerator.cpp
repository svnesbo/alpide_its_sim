//-----------------------------------------------------------------------------
// Title      : Alpide Data generator
// Project    : ALICE ITS WP10
//-----------------------------------------------------------------------------
// File       : alpide_gen.cpp
// Author     : Matthias Bonora (matthias.bonora@cern.ch)
// Company    : CERN / University of Salzburg
// Created    : 2015-11-13
// Last update: 2015-11-13
// Platform   : CERN 7 (CentOs)
// Target     : Simulation
// Standard   : SystemC 2.3
//-----------------------------------------------------------------------------
// Description:
//-----------------------------------------------------------------------------
// Copyright (c)   2015
//-----------------------------------------------------------------------------
// Revisions  :
// Date        Version  Author        Description
// 2015-11-13  1.0      mbonora        Created
//-----------------------------------------------------------------------------

#include "AlpideDataGenerator.hpp"

#include <cassert>
#include <bitset>
#include <set>
#include <map>
#include <algorithm>
#include <iterator>
#include <numeric>
#include <string>
#include <fstream>
#include <iostream>

using namespace std;

void inner_barrel_test(int const NR_EVENTS, int const chipId,
                       std::string outputFileName) {}

void AlpideDataGenerator::idle() { mData.push_back(0xFF); }
void AlpideDataGenerator::busyOn() { mData.push_back(0xF1); }
void AlpideDataGenerator::busyOff() { mData.push_back(0xF0); }
void AlpideDataGenerator::chipHeader(size_t chipId, size_t frameTimestamp) {
  assert(chipId <= 14);
  assert(frameTimestamp < 256);

  mData.push_back(0xA0 | static_cast<uint8_t>(chipId));
  mData.push_back(static_cast<uint8_t>(chipId));
}

void AlpideDataGenerator::comma(bool innerBarrel) {
  int target = (innerBarrel) ? 3 : 1;
  for (int i = 0; i < target; ++i)
    mData.push_back(0xBC);
}

/*
Flags:
- 0 BUSY_TRANSITION
- 1 FATAL (panic mode)
- 2 FLUSHED_FRAME (in continuous mode)
- 3 BUSY_VIOLATION (in triggered mode)
*/
inline void AlpideDataGenerator::chipTrailer(bool busyTransition, bool fatal,
                                             bool flushedFrame,
                                             bool busyViolation) {
  std::bitset<8> data(0xB0);
  data.set(0, busyTransition);
  data.set(1, fatal);
  data.set(2, flushedFrame);
  data.set(3, busyViolation);

  mData.push_back(static_cast<uint8_t>(data.to_ulong()));
  idle(); // append idle
}
inline void AlpideDataGenerator::chipEmptyFrame(size_t chipId,
                                                size_t frameTimestamp) {
  assert(chipId <= 14);
  assert(frameTimestamp < 256);

  mData.push_back(0xE0 | static_cast<uint8_t>(chipId));
  mData.push_back(static_cast<uint8_t>(chipId));
  idle();
}
inline void AlpideDataGenerator::regionHeader(size_t regionId) {
  assert(regionId < 32);
  mData.push_back(0xC0 | static_cast<uint8_t>(regionId));
}
inline void AlpideDataGenerator::dataShort(size_t hitPosition) {
  assert(hitPosition < (1 << 14));

  uint8_t dataHigh = 0x40 | static_cast<uint8_t>((hitPosition >> 8) & 0x3F);
  uint8_t dataLow = static_cast<uint8_t>(0xFF & hitPosition);

  mData.push_back(dataHigh);
  mData.push_back(dataLow);
}
inline void AlpideDataGenerator::dataLong(size_t hitPosition, size_t hitMap) {
  assert(hitPosition < (1 << 14));
  assert(hitMap < 128);

  uint8_t dataHigh = 0x00 | static_cast<uint8_t>((hitPosition >> 8) & 0x3F);
  uint8_t dataLow = static_cast<uint8_t>(0xFF & hitPosition);

  mData.push_back(dataHigh);
  mData.push_back(dataLow);
  mData.push_back(static_cast<uint8_t>(hitMap));
}

void AlpideDataGenerator::clearData() { mData.clear(); }
std::vector<uint8_t> AlpideDataGenerator::getData() { return mData; }

void AlpideDataGenerator::generateChipHit(size_t chipId, size_t frameTimestamp,
                                          bool innerBarrel) {

  static std::default_random_engine generator;

  auto event_gen = std::bind(dist_event, generator);
  auto dataShort_gen = [this]()->int {
    int val = *random_dataShort_it++;
    if (random_dataShort_it == end(random_dataShort))
      random_dataShort_it = begin(random_dataShort);
    return val;
  }; // std::bind(dist_dataShort,generator);
  auto dataLong_gen = [this]()->int {
    int val = *random_dataLong_it++;
    if (random_dataLong_it == end(random_dataLong))
      random_dataLong_it = begin(random_dataLong);
    return val;
  }; // std::bind(dist_dataLong,generator);

  auto hit_gen = [this]()->int {
    int val = *random_hits_it++;
    if (random_hits_it == end(random_hits))
      random_hits_it = begin(random_hits);
    return val;
  }; // std::bind(dist_hits,generator);

  bool emptyChipEvent = true;

  // generate Events for each region
  for (size_t i = 0; i < 32; ++i) {
    // Events per Region
    int nrEvents = event_gen();
    bool firstEvent = true;

    uint32_t fastHitIdx = 0;

    std::vector<uint32_t> regionMap;
    regionMap.reserve(10000);
    for (int j = 0; j < nrEvents; ++j) {
      int shortDataEvents = dataShort_gen();
      int longDataEvents = dataLong_gen();
      for (int k = 0; k < shortDataEvents; ++k) {
        if (m_fastGen) {
          auto hitIdx = fastHitIdx++;
          regionMap.push_back(hitIdx);
        } else {
          uint32_t hitIdx = hit_gen();
          regionMap.push_back(hitIdx);
        }
      }
      for (int k = 0; k < longDataEvents; ++k) {
        if (m_fastGen) {
          if (fastHitIdx % 1024 >= 1022)
            fastHitIdx += 2;
          auto hitIdx = fastHitIdx;
          regionMap.push_back(hitIdx);
          fastHitIdx += 8;
        } else {
          uint32_t hitIdx = hit_gen();
          regionMap.push_back(hitIdx);
          if (hitIdx + 1 < 16 * 1024)
            regionMap.emplace_back(hitIdx + 1);
        }
      }
    }

    if (!m_fastGen) {
      std::sort(begin(regionMap), end(regionMap));
      auto it_unique = std::unique(begin(regionMap), end(regionMap));
      regionMap.erase(it_unique, end(regionMap));
    }
    // make events
    auto it = begin(regionMap);

    while (it != end(regionMap)) {
      // hit, check future hits
      size_t roIdx = *it;
      std::bitset<8> hitmap;
      hitmap.set(7);
      auto it_next = it + 1;
      while (it_next != end(regionMap) && *it_next < roIdx + 8) {
        size_t hitIdx = *it_next - roIdx;
        hitmap[7 - hitIdx] = true;
        ++it_next;
      }
      it = it_next;
      // create Event entries
      if (firstEvent) {
        if (emptyChipEvent) {
          chipHeader(chipId, frameTimestamp);
          if (innerBarrel)
            idle();
          emptyChipEvent = false;
        }

        regionHeader(i);
        if (innerBarrel) {
          idle();
          idle();
        }
        firstEvent = false;
      }

      if (hitmap.count() > 1) {
        size_t hits = static_cast<size_t>(hitmap.to_ulong()) & ~(0x80);
        dataLong(roIdx, hits);
      } else {
        dataShort(roIdx);
        if (innerBarrel)
          idle();
      }
    }
    /*
        auto const regionMapSize = regionMap.size();
        while (roIdx < regionMapSize) {
          if (regionMap[roIdx]) {
            // check following hits
            std::bitset<8> hitmap;
            for (int j = 0; j < 8 && j + roIdx < regionMapSize; ++j) {
              hitmap[7-j] = regionMap[roIdx + j];
            }

            // create Event entries
            if (firstEvent) {
              if (emptyChipEvent) {
                chipHeader(chipId, frameTimestamp);
                if (innerBarrel)
                  idle();
                emptyChipEvent = false;
              }

              regionHeader(i);
              if (innerBarrel) {
                idle();
                idle();
              }
              firstEvent = false;
            }

            if (hitmap.count() > 1) {
              size_t hits = static_cast<size_t>(hitmap.to_ulong())&~(0x80);
              dataLong(roIdx, hits);
            } else {
              dataShort(roIdx);
              if (innerBarrel)
                idle();
            }
            roIdx += 8;
          } else {
            ++roIdx;
          }
        }
    */
  }

  if (emptyChipEvent) {
    chipEmptyFrame(chipId, frameTimestamp);
  } else {
    chipTrailer();
    if (innerBarrel)
      idle();
  }
}
