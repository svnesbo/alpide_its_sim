/**
 * @file   main.cpp
 * @Author Simon Voigt Nesbo
 * @date   December 5, 2016
 * @brief  Main source file for simulation testbench
 *
 * Detailed description of file.
 */

#include "simulator.h"
#include <set>

// Example simulation modes
enum SimulationModes{ACCURATE_MODEL, TOY_MODEL};

int main(int argc, char** argv)
{
  // Parse configuration file here

  // Initialize variables, event generator and detector based on configuration file
  
  std::set<int> layers = {0, 1, 2, 3, 4, 5, 6};  
  EventGenerator* events = new EventGenerator();
  ITSDetector* detector = new ITSDetector(layers);

  SimulationMode sim_mode = TOY_MODEL;
  int num_events = 1000;


  switch(sim_mode) {
  case ACCURATE_MODEL:
    // Run simulation in one particular way, with one particular set of params
    sim(detector, events, num_events);
    break;
  case TOY_MODEL:
    // Run simulation with different params
    break;
  case SIMULATION_MODE3:
    // Run simulation with different params    
    break;    
  }
  
}
