//-----------------------------------------------------------------------------
// Title      : Alpide-3 Data generator
// Project    : ALICE ITS WP10
//-----------------------------------------------------------------------------
// File       : alpide_gen.hpp
// Author     : Matthias Bonora (matthias.bonora@cern.ch)
// Company    : CERN / University of Salzburg
// Created    : 2015-11-13
// Last update: 2015-11-13
// Platform   : CERN 7 (CentOs)
// Target     : Simulation
// Standard   : SystemC 2.3
//-----------------------------------------------------------------------------
// Description: Generate Alpide-3 conform Data stream packets
//-----------------------------------------------------------------------------
// Copyright (c)   2015
//-----------------------------------------------------------------------------
// Revisions  :
// Date        Version  Author        Description
// 2015-11-13  1.0      mbonora        Created
//-----------------------------------------------------------------------------

#pragma once

#include <vector>
#include <cstdint>

#include <algorithm>
#include <random>

class AlpideDataGenerator {
public:


  AlpideDataGenerator(double avgHitsPerRegion, double avgDataShortPerHit, double avgDataLongPerHit,bool fastGen=true) :
    dist_event(avgHitsPerRegion),
    dist_dataShort(avgDataShortPerHit),
    dist_dataLong(avgDataLongPerHit),
    dist_hits(0,16*1024-1),
    m_fastGen(fastGen) {

    std::default_random_engine generator;
    random_hits.resize(1000);
    std::generate(begin(random_hits),end(random_hits),std::bind(dist_hits,generator));
    random_hits_it=begin(random_hits);

    random_dataShort.resize(1000);
    std::generate(begin(random_dataShort),end(random_dataShort),std::bind(dist_dataShort,generator));
    random_dataShort_it=begin(random_dataShort);

    random_dataLong.resize(1000);
    std::generate(begin(random_dataLong),end(random_dataLong),std::bind(dist_dataLong,generator));
    random_dataLong_it=begin(random_dataLong);
  }



  void generateChipHit(std::size_t chipId, std::size_t frameTimestamp, bool innerBarrel);

  void idle();
  void comma(bool innerBarrel = false);
  void busyOn();
  void busyOff();
  void chipHeader(std::size_t chipId, std::size_t frameTimestamp);
  void chipTrailer(bool busyTransition = false, bool fatal = false,
                   bool flushedFrame = false, bool busyViolation = false);
  void chipEmptyFrame(std::size_t chipId, std::size_t frameTimestamp);
  void regionHeader(std::size_t regionId);
  void dataShort(std::size_t hitPosition);
  void dataLong(std::size_t hitPosition, std::size_t hitMap);

  void clearData();
  std::vector<uint8_t> getData();

private:
  std::vector<uint8_t> mData;

  std::poisson_distribution<int> dist_event;
  std::poisson_distribution<int> dist_dataShort;
  std::poisson_distribution<int> dist_dataLong;
  std::uniform_int_distribution<int> dist_hits;

  std::vector<int> random_hits;
  std::vector<int>::const_iterator random_hits_it;
  std::vector<int> random_dataShort;
  std::vector<int>::const_iterator random_dataShort_it;
  std::vector<int> random_dataLong;
  std::vector<int>::const_iterator random_dataLong_it;

  bool m_fastGen;
};
