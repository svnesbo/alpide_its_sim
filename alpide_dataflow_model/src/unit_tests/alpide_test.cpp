#include "../alpide/alpide.h"
#define BOOST_TEST_MODULE AlpideTest
#include <boost/test/included/unit_test.hpp>
#include <vector>


BOOST_AUTO_TEST_CASE( alpide_test )
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

  // Just a dummy counter value for every time a simulation time is needed
  long event_time = 0;

  BOOST_TEST_MESSAGE("Setting up Alpide SystemC simulation");
  
  Alpide alpide("alpide",
                0,
                128,
                64,
                enable_clustering,
                continuous_mode);

  AlpideDataParser parser("parser");

  // Initialize SystemC stuff and connect signals to Alpide here
  parser.serial_data_in(alpide.s_serial_data_output);
  
  // Start/run for x number of clock cycles
  sc_core::sc_start();


  BOOST_TEST_MESSAGE("Creating event with 100 random hits");
  
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
  sc_core::sc_start();

  // By now the chip object should have finished transmitting the hits,
  // so the parser should have one full hit
  BOOST_TEST_MESSAGE("Checking that the chip has transmitted 1 full event.");
  BOOST_CHECK_EQUAL(parser.getNumEvents(), 1);

  BOOST_TEST_MESSAGE("Checking that the parsed event has the right amount ofhits.");
  BOOST_CHECK_EQUAL(parser.getEvent(0).getEventSize(), hit_vector.size());

  BOOST_TEST_MESSAGE("Checking that the event contains all the hits that were generated, and nothimg more.");  
  while(!hit_vector.empty()) {
    BOOST_CHECK_EQUAL(parser.getEvent(0).hitInEvent(hit_vector.back()), true);
    hit_vector.pop_back();
  }

  
  ///@todo Create more advanced tests of Alpide chip here.. test that clusters are generated
  ///      correctly, that data is read out sufficiently fast, etc.
}
