#include "AlpideBase.hpp"

//#include <boost/format.hpp>
#include <functional>
#include <future>
#include <thread>


using namespace Alpide;
using std::placeholders::_1;


InnerBarrelModule::InnerBarrelModule(sc_core::sc_module_name const &name)
  : Module(name)
  , control("control")
{
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


ControlResponsePayload InnerBarrelModule::processCommand(const ControlRequestPayload &request)
{
  ControlResponsePayload b;
  // SC_REPORT_INFO_VERB(name(), "Received Command", sc_core::SC_DEBUG);
  for (size_t i = 0; i < m_chipControlLinks.size(); ++i) {
    auto result = m_chipControlLinks[i]->transport(request);
    if (request.chipId == i)
      b = result;
  }

  return b;
}
