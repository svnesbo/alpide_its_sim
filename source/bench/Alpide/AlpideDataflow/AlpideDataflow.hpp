/**
 * @file   AlpideDataflow.hpp
 * @author Simon Voigt Nesbo
 * @date   May 22, 2017
 * @brief  Top level header file for Alpide Dataflow model, which adds a nice control and data
 *         socket interface to the class, common to both the Dataflow and the Simple Alpide models.
 */

#include "Alpide.hpp"
#include "AlpideBase.hpp"


namespace Alpide {

  class AlpideDataflow : public AlpideBase, protected Alpide {
  public:
    SC_HAS_PROCESS(AlpideDataflow);
    AlpideDataflow(sc_core::sc_module_name const &name = 0, int chip_id = 0);
    ControlResponsePayload processCommand(ControlRequestPayload const &request);
  };

}
