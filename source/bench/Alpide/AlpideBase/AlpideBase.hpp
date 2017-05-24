/**
 * @file   AlpideBase.hpp
 * @author Simon Voigt Nesbo
 * @date   May 22, 2017
 * @brief  Base class for Alpide Dataflow and Simple models.
 */

#pragma once

#include <systemc>
#include <AlpideInterface.hpp>

using sc_core::sc_module;

namespace Alpide {
  struct Module : public sc_module {
    Module(sc_core::sc_module_name const &name = 0) : sc_module(name) {}
  };

  class AlpideBase : public Module {
    ControlTargetSocket control;
    DataInitiatorSocket data;

    virtual ControlResponsePayload processCommand(ControlRequestPayload const &request) = 0;

  public:
    AlpideBase(sc_core::sc_module_name const &name = 0) : Module(name) {}
  };


  /// TODO: Move the modules to a separate file?? Make the module able to use both
  ///       AlpideSimple and AlpideDataflow models??
  struct InnerBarrelModule : Module {
    InnerBarrelModule(sc_core::sc_module_name const &name = 0);
    ControlTargetSocket control;
    std::array<DataInitiatorSocket, 9> data;

  private:
    ControlResponsePayload processCommand(ControlRequestPayload const &request);

    /// Create 9 instances of AlpideBase here?

    std::vector<std::unique_ptr<SingleChip>> m_chips;
    std::array<ControlInitiatorSocket,9> m_chipControlLinks;
  };

  struct OuterBarrelModule : Module {
    OuterBarrelModule(sc_core::sc_module_name const &name = 0) : Module(name) {}
    std::array<ControlTargetSocket, 4> control;
    std::array<DataInitiatorSocket, 28> data;

  private:
    /// Create X instances of AlpideBase here?
  };
}
