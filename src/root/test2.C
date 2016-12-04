//#include "../alpide/pixel_col.h"
#include "../event/hit.h"
#include "../event/event.h"
#include "../event/event_generator.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <iterator>
#include <queue>
#include "TGraph.h"
#include "TCanvas.h"
#include "TH1.h"
#include "TRandom3.h"


// time_vector is placed on heap to avoid stack overflow.
const int n_events = 5000;


void test2(){
  const float mean_count=3.6;
  const double BC_period_ns = 25;
  const double gap_factor = 0.0;
  const double hit_mult_avg = 1000;
  const double hit_mult_stddev = 300;
  const double random_seed = 12345;

  EventGenerator *events = new EventGenerator(BC_period_ns,
                                              gap_factor,
                                              hit_mult_avg,
                                              hit_mult_stddev,
                                              random_seed);
  
  Hit h1(10, 3, 100, 0, 1000);
  Hit h2(20, 6, 13);
  Hit h3(10, 9, 235);
  Hit h4(10, 3, 100);
  Hit h5(1234, 12, 23042);
  Hit h6 = h5;
  Hit h7(h4);

  std::cout << "h1 == h2: " << ((h1 == h2) ? "true" : "false") << std::endl;
  std::cout << "h1 == h3: " << ((h1 == h3) ? "true" : "false") << std::endl;
  std::cout << "h1 == h4: " << ((h1 == h4) ? "true" : "false") << std::endl;
  std::cout << "h1 == h5: " << ((h1 == h5) ? "true" : "false") << std::endl;
  std::cout << "h1 == h6: " << ((h1 == h6) ? "true" : "false") << std::endl;
  std::cout << "h1 == h7: " << ((h1 == h7) ? "true" : "false") << std::endl;  

  // Generate an event
  events->generateNextEvent();

  events->generateNextEvents(n_events-1);

  int prev_event_time_ns = 0;

  // Iterate over all events
  while(events->getEventsInMem() > 0) {
    const Event& event = events->getNextEvent();
    
    int carried_over_count = event.getCarriedOverCount();
    int not_carried_over_count = event.getNotCarriedOverCount();
    int current_event_time_ns = event.getEventTime();
    int t_delta = current_event_time_ns - prev_event_time_ns;
    int n_hits = event.getEventSize();
    int new_hits = event.getEventSize() - event.getCarriedOverCount();

    std::cout << "Event " << event.getEventId() << ":\t @ " << current_event_time_ns << " ns\t (t_delta = " << t_delta << " ns). \t";
    std::cout << carried_over_count << "/" << carried_over_count+not_carried_over_count << " hits carried over from previous event. ";
    std::cout << new_hits << " new hits. " <<  n_hits << " hits in total." << std::endl;

    prev_event_time_ns = current_event_time_ns;
    events->removeOldestEvent();
  }
  
}
