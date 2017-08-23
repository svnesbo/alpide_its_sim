/**
 * @file   ITSModulesStaves.cpp
 * @author Simon Voigt Nesbo
 * @date   August 15, 2017
 * @brief  This file holds a collection of classes and structs that define modules,
 *         staves and barrels/layers in the ITS detector.
 *         Much of this code is copy-paste from AlpideModule.hpp/cpp in
 *         Matthias Bonora's WP10 RU SystemC simulation code.
 */

#include "ITSModulesStaves.hpp"
#include "ITS_config.hpp"

using namespace ITS;
//using std::placeholders::_1;

InnerBarrelStave::InnerBarrelStave(sc_core::sc_module_name const &name,
                                   unsigned int layer_id,
                                   unsigned int stave_id)
  : StaveInterface(name, layer_id, stave_id, 1, 9)
{
  socket_control_in[0].register_transport(
    std::bind(&InnerBarrelStave::processCommand, this, std::placeholders::_1));

  for (int i = 0; i < 9; i++) {
    int chip_id = detector_position_to_chip_id({layer_id, stave_id, 0, i});
    std::string name = "Chip_" + std::to_string(chip_id);

    std::cout << "Creating chip with ID " << chip_id << std::endl;

    mChips.push_back(std::make_shared<Alpide>(name.c_str(), chip_id, 128, 64, 0, 100, false, true, false, true));

    // Alpide(sc_core::sc_module_name name, int chip_id, int region_fifo_size,
    //        int dmu_fifo_size, int dtu_delay_cycles, int strobe_length_ns,
    //        bool strobe_extension, bool enable_clustering, bool continuous_mode,
    //        bool matrix_readout_speed);

    auto &chip = *mChips.back();
    socket_control_out[i].bind(chip.s_control_input);
    chip.s_data_output(socket_data_out[i]);
    chip.s_system_clk_in(s_system_clk_in);
  }
}


ControlResponsePayload
InnerBarrelStave::processCommand(const ControlRequestPayload &request) {
  ControlResponsePayload b;
  // SC_REPORT_INFO_VERB(name(), "Received Command", sc_core::SC_DEBUG);
  for (size_t i = 0; i < socket_control_out.size(); ++i) {
    auto result = socket_control_out[i]->transport(request);
    if (request.chipId == i)
      b = result;
  }

  return b;
}
