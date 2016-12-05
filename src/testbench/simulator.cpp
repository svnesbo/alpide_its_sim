/**
 * @file   simulator.cpp
 * @Author Simon Voigt Nesbo
 * @date   December 5, 2016
 * @brief  Main source file for simulation testbench
 *
 * Detailed description of file.
 */

#include "simulator.h"

 
void sim(ITSDetector* detector, EventGenerator* events, int num_events)
{
  bool simulation_done = false;
  int time_ns = 0;


  while(!simulation_done) {
    events->removeOldestEvent();
    events->generateNextEvent();

    Event& e = events->getNextEvent();

    // Wait for next event to occur
    while(time_ns < e.getEventTime()) {
      time_ns = wait_for_clk_edge();
    }

    detector->feedEventsToChips(e);

    // Should we allow the simulation to run for a little while after the last event has been generated?
    // That would allow for buffers etc. to empty. If that's done then we also have to make a check at the
    // beginning of this loop that new events are not created after the event count has been reached. 
    if(events->getEventCount() == num_events)
      simulation_done = true;
  }

}
