/**
 * @file   simulator.h
 * @Author Simon Voigt Nesbo
 * @date   December 5, 2016
 * @brief  Header file for simulation testbench
 *
 * Detailed description of file.
 */

#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "../event/event_generator.h"
#include "../its_detector/build_detector.h"

void sim(ITSDetector* detector, EventGenerator* events, int num_events);


#endif
