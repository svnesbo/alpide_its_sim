//-----------------------------------------------------------------------------
// Title      : Alpide Interface
// Project    : ALICE ITS WP10
//-----------------------------------------------------------------------------
// File       : AlpideControlPayload.hpp
// Author     : Matthias Bonora (matthias.bonora@cern.ch)
// Company    : CERN / University of Salzburg
// Created    : 2017-03-27
// Last update: 2017-03-27
// Platform   : CERN 7 (CentOs)
// Target     : Simulation
// Standard   : SystemC 2.3
//-----------------------------------------------------------------------------
// Description: Interface + Payload description for Alpide chip connection
//-----------------------------------------------------------------------------
// Copyright (c)   2017
//-----------------------------------------------------------------------------
// Revisions  :
// Date        Version  Author        Description
// 2017-03-27  1.0      mbonora        Created
//-----------------------------------------------------------------------------

#pragma once

#include <cstdint>
#include <vector>
#include <memory>

#include <systemc>
#include <tlm>

#include <common/Interfaces.hpp>
#include <event/EventFrame.hpp>

namespace Alpide {
  struct ControlRequestPayload {
    uint8_t opcode;
    uint8_t chipId;
    uint16_t address;
    uint16_t data;
  };

  struct ControlResponsePayload {
    uint8_t chipId;
    uint16_t data;
  };

  struct DataPayload {
    std::vector<uint8_t> data;
  };

  using ControlInitiatorSocket = sc_core::sc_port<
    tlm::tlm_transport_if<ControlRequestPayload, ControlResponsePayload> >;
  using ControlTargetSocket =
    transport_target_socket<ControlRequestPayload, ControlResponsePayload>;

  using DataInitiatorSocket =
    sc_core::sc_port<tlm::tlm_blocking_put_if<DataPayload> >;
  using DataTargetSocket = put_if_target_socket<DataPayload>;
  using DataTargetExport =
    sc_core::sc_export<tlm::tlm_blocking_put_if<DataPayload> >;
}
