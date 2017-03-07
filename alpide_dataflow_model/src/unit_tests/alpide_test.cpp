/**
 * @file   alpide_test.cpp
 * @author Simon Voigt Nesbo
 * @date   March 7, 2017
 * @brief  "Unit test" for the whole Alpide chip.
 *         Boost test could presumably not be used for this test, because both SystemC and boost test 
 *         expects to be in charge of main.
 *         Anyway, the test does the following:
 *         1) Sets up an Alpide chip object
 *         2) Sets up an AlpideDataParser object
 *         3) Sets up necessary clocks and signals, and connects the alpide and parser objects.
 *         4) Creates an event with some hits, and feeds it to the alpide
 *         5) Starts the SystemC simulation and lets it run for a little while
 *         6) Verifies that the parser has received the event and that the hits are the same
 */

// Ignore warnings about use of auto_ptr in SystemC library
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include "../alpide/alpide.h"
#include "../alpide/alpide_data_parser.h"
#include <boost/random/random_device.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <vector>
#include <iostream>


//BOOST_AUTO_TEST_CASE( alpide_test )
int sc_main(int argc, char** argv)
{
  boost::random::mt19937 rand_gen;
  boost::random::uniform_int_distribution<int> rand_x_dist(0, N_PIXEL_COLS-1);
  boost::random::uniform_int_distribution<int> rand_y_dist(0, N_PIXEL_ROWS-1);
  int rand_x, rand_y;
  std::vector<Hit> hit_vector;
  
  int chip_id = 0;
  int event_id = 0;
  bool continuous_mode = false;
  bool enable_clustering = true;

  std::cout << "Setting up Alpide SystemC simulation" << std::endl;

  // Setup SystemC stuff
  Alpide alpide("alpide",
                0,
                128,
                64,
                enable_clustering,
                continuous_mode);

  AlpideDataParser parser("parser");

  sc_trace_file *wf = NULL;
  sc_core::sc_set_time_resolution(1, sc_core::SC_NS);
  
  // 25ns period, 0.5 duty cycle, first edge at 2 time units, first value is true
  sc_clock clock_40MHz("clock_40MHz", 25, 0.5, 2, true);

  int matrix_readout_period = 50; // 50 ns
  sc_clock clock_matrix_readout("clock_matrix_readout",
                                matrix_readout_period,
                                0.5, 2, true);
  sc_signal<sc_uint<24> > alpide_serial_data;

  alpide.s_system_clk_in(clock_40MHz);
  alpide.s_matrix_readout_clk_in(clock_matrix_readout);
  alpide.s_serial_data_output(alpide_serial_data);

  // Initialize SystemC stuff and connect signals to Alpide here
  parser.s_serial_data_in(alpide_serial_data);
  parser.s_clk_in(clock_40MHz);
  
  // Start/run for x number of clock cycles
  sc_core::sc_start(1000, sc_core::SC_NS);


  std::cout << "Creating event with 100 random hits" << std::endl;
  
  int64_t time_now = sc_time_stamp().value();
  
  // Create a trigger event object
  TriggerEvent e(time_now, time_now+1000, chip_id, event_id++);
  
  // Create 100 random hits
  for(int i = 0; i < 100; i++) {
    rand_x = rand_x_dist(rand_gen);
    rand_y = rand_y_dist(rand_gen);

    // Store hits in vector - used later to check against data from serial output
    hit_vector.emplace_back(rand_x, rand_y, time_now, time_now+1000);

    // Store hit in trigger event object
    e.addHit(hit_vector.back());
  }

  // Create an event in the Alpide
  alpide.newEvent(time_now);

  // Feed trigger event to Alpide
  e.feedHitsToChip(alpide);


  // Start/run for x number of clock cycles
  sc_core::sc_start(100000, sc_core::SC_US);

  // By now the chip object should have finished transmitting the hits,
  // so the parser should have one full hit
  std::cout << "Checking that the chip has transmitted 1 full event." << std::endl;
  if(parser.getNumEvents() == 1) {
    std::cout << "Alpide parser correctly contains 1 event." << std::endl;
  } else {
    std::cout << "Error: Alpide parser contains " << parser.getNumEvents();
    std::cout << " events, should have 1." << std::endl;
  }

  const AlpideEventFrame* event = parser.getNextEvent();
  if(event == nullptr) {
    std::cout << "Error: parser returned null pointer for next event." << std::endl;
    return -1;
  }

  std::cout << "Checking that the parsed event has the right amount of hits." << std::endl;
  if(event->getEventSize() == hit_vector.size()) {
    std::cout << "Alpide parser event size correctly matches input hit vector size." << std::endl;
  } else {
    std::cout << "Error: Alpide parser event size is " << event->getEventSize();
    std::cout << ", should equal input hit vector size which is " << hit_vector.size();
    std::cout << "." << std::endl;
  }  

  std::cout << "Checking that the event contains all the hits that were generated, and nothimg more." << std::endl;
  while(!hit_vector.empty()) {
    if(event->pixelHitInEvent(hit_vector.back()) == false) {
      std::cout << "Error: missing pixel " << hit_vector.back().getCol() << ":";
      std::cout << hit_vector.back().getRow() << " in Alpide parser event." << std::endl;
    }
    hit_vector.pop_back();
  }

  sc_core::sc_stop();

  
  ///@todo Create more advanced tests of Alpide chip here.. test that clusters are generated
  ///      correctly, that data is read out sufficiently fast, etc.
}
