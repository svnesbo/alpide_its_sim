//-----------------------------------------------------------------------------
// Title      : Alpide Module
// Project    : ALICE ITS WP10
//-----------------------------------------------------------------------------
// File       : AlpideModule.hpp
// Author     : Matthias Bonora (matthias.bonora@cern.ch)
// Company    : CERN / University of Salzburg
// Created    : 2017-03-28
// Last update: 2017-03-28
// Platform   : CERN 7 (CentOs)
// Target     : Simulation
// Standard   : SystemC 2.3
//-----------------------------------------------------------------------------
// Description: An Alpide Module for ()
//-----------------------------------------------------------------------------
// Copyright (c)   2017
//-----------------------------------------------------------------------------
// Revisions  :
// Date        Version  Author        Description
// 2017-03-28  1.0      mbonora        Created
//-----------------------------------------------------------------------------

#pragma once

#include <functional>
#include <future>
#include <array>
#include <memory>

#include <systemc>

#include <AlpideBase/AlpideBase.hpp>
#include <AlpideSimple/AlpideDataGenerator.hpp>

using sc_core::sc_module;

namespace Alpide {

  /// TODO: Keep chip class in a different class than the modules??
  class AlpideSimple : public AlpideBase {
    uint8_t m_chipId;
    AlpideDataGenerator m_datagen;

    std::future<void> m_generateFuture;
    sc_core::sc_event m_hitGeneratedEvent;

  public:
    SC_HAS_PROCESS(AlpideSimple);
    SingleChip(sc_core::sc_module_name const &name = 0, uint8_t chipId = 0);

    ControlResponsePayload processCommand(ControlRequestPayload const &request);

    void generateEvent();
    void sendEvent();
  };

}
