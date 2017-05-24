#include "AlpideDataflow.hpp"

using namespace Alpide;
using std::placeholders::_1;

AlpideDataflow::AlpideDataflow(sc_core::sc_module_name const &name, uint8_t chipId)
  : Module(name), control("control"), data("data"), m_chipId(chipId),
    m_datagen(100, 1, 1) {
  control.register_transport(
    std::bind(&AlpideDataflow::processCommand, this, std::placeholders::_1));
  SC_THREAD(sendEvent);
}


ControlResponsePayload AlpideDataflow::processCommand(ControlRequestPayload const &request)
{
  if (request.opcode == 0x55) {
    // SC_REPORT_INFO_VERB(name(), "Received Trigger", sc_core::SC_DEBUG);
    generateEvent();
  } else {
    SC_REPORT_ERROR(name(), "Invalid opcode received");
  }
  // do nothing
  return {};
}
