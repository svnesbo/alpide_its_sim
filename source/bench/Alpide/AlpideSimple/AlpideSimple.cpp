//-----------------------------------------------------------------------------
// Title      : Alpide Module Implementations
// Project    : ALICE ITS WP10
//-----------------------------------------------------------------------------
// File       : AlpideModule.cpp
// Author     : Matthias Bonora (matthias.bonora@cern.ch)
// Company    : CERN / University of Salzburg
// Created    : 2017-05-02
// Last update: 2017-05-02
// Platform   : CERN 7 (CentOs)
// Target     : Simulation
// Standard   : SystemC 2.3
//-----------------------------------------------------------------------------
// Description: Implementations for the different Alpide modules
//-----------------------------------------------------------------------------
// Copyright (c)   2017
//-----------------------------------------------------------------------------
// Revisions  :
// Date        Version  Author        Description
// 2017-05-02  1.0      mbonora        Created
//-----------------------------------------------------------------------------

#include "AlpideSimple.hpp"

#include <boost/format.hpp>
#include <functional>
#include <future>
#include <thread>


using namespace Alpide;
using std::placeholders::_1;

AlpideSimple::AlpideSimple(sc_core::sc_module_name const &name, uint8_t chipId)
  : Module(name), control("control"), data("data"), m_chipId(chipId),
    m_datagen(100, 1, 1) {
  control.register_transport(
    std::bind(&AlpideSimple::processCommand, this, std::placeholders::_1));
  SC_THREAD(sendEvent);
}

ControlResponsePayload
AlpideSimple::processCommand(ControlRequestPayload const &request) {
  if (request.opcode == 0x55) {
    // SC_REPORT_INFO_VERB(name(), "Received Trigger", sc_core::SC_DEBUG);
    generateEvent();
  } else {
    SC_REPORT_ERROR(name(), "Invalid opcode received");
  }
  // do nothing
  return {};
}

void AlpideSimple::generateEvent() {
  m_datagen.clearData();
//  m_generateFuture = std::async(std::launch::deferred, [this]() {
    m_datagen.generateChipHit(m_chipId, 0, true);
//  });
//  m_hitGeneratedEvent.notify(0, sc_core::SC_NS);
    data->put({ m_datagen.getData() });
}

void AlpideSimple::sendEvent() {
  while(true) {
    sc_core::wait(m_hitGeneratedEvent);
//    m_generateFuture.get();
    data->put({ m_datagen.getData() });
  }
}

InnerBarrelModule::InnerBarrelModule(sc_core::sc_module_name const &name)
    : Module(name), control("control") {
  control.register_transport(
      std::bind(&InnerBarrelModule::processCommand, this, _1));
  for (int i = 0; i < 9; ++i) {
    std::string name = "Chip_" + std::to_string(i);
    m_chips.emplace_back(new AlpideSimple(name.c_str(), i));
    auto &chip = *m_chips.back();
    m_chipControlLinks[i].bind(chip.control);
    chip.data(data[i]);
  }
}

ControlResponsePayload
InnerBarrelModule::processCommand(const ControlRequestPayload &request) {
  ControlResponsePayload b;
  // SC_REPORT_INFO_VERB(name(), "Received Command", sc_core::SC_DEBUG);
  for (size_t i = 0; i < m_chipControlLinks.size(); ++i) {
    auto result = m_chipControlLinks[i]->transport(request);
    if (request.chipId == i)
      b = result;
  }

  return b;
}
