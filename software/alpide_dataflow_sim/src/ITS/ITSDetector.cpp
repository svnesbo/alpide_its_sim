/**
 * @file   ITSDetector.cpp
 * @author Simon Voigt Nesbo
 * @date   June 21, 2017
 * @brief  Mockup version of ITS detector.
 *         Accepts trigger input from the dummy CTP module, and communicates the trigger
 *         to the readout units, which will forward them to the Alpide objects.
 */


#include "ITSDetector.hpp"


//Do I need SC_HAS_PROCESS if I only use SC_METHOD??
//SC_HAS_PROCESS(ITSDetector);
ITSDetector::ITSDetector(sc_core::sc_module_name name)
  : sc_core::sc_module(name)
{
  ///@todo Construct ITS detector here.
  // In the constructor, we should specify which layers, and how many staves (readout units)
  // for each layer, that we want to include in the detector/simulation.
  // We don't specify number of Alpide chips, we always construct full staves.
  // Declare readout units and Alpide chips, and connect them together.
}
