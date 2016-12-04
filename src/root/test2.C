//#include "../alpide/pixel_col.h"
#include "../event/hit.h"
#include "../event/event.h"
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
const int n_events = 10000;
double time_vector[n_events];
double x_vector[n_events];
//std::vector<Hit> hit_vectors[n_events];


std::queue<Event*> event_queue;


//std::vector<double> time_vector;
//std::vector<int> x_vector;

void test2(){
  TH1F* cnt_r_h = new TH1F("count_rate",
                           "Count Rate;N_{Counts};# occurencies",
                           100, // Number of Bins
                           0, // Lower X Boundary
                           10000); // Upper X Boundary

  TH1F* hit_distribution = new TH1F("hit_distribution",
                                    "Hit distribution;N_{Counts};# occurencies",
                                    100, // Number of Bins
                                    0, // Lower X Boundary
                                    20); // Upper X Boundary  

  const float mean_count=3.6;
  const double BC_period_ns = 25;
  const double hit_mult_avg = 1000;
  const double hit_mult_stddev = 300;

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

  TRandom hitgen;
  hitgen.SetSeed(0);   // Use random seed

  TRandom hit_multiplicity_gen;
  hit_multiplicity_gen.SetSeed(0); // Use random seed

  TRandom rndgen;
  rndgen.SetSeed(0);   // Use random seed

  double t_delta;

  // simulate the measurements
  for(int event = 0; event < n_events; event++) {
    // Generate random (exp distributed) interval till next event/interaction
    t_delta = 100*rndgen.Exp(BC_period_ns);

    // Add interval to histogram
    cnt_r_h->Fill(t_delta);


    // Build event time vector
    if(event == 0)
      time_vector[event] = t_delta;
    else
      time_vector[event] = time_vector[event-1] + t_delta;


    x_vector[event] = event;

    Event *next_event = new Event(time_vector[event], event);
    
    if(event > 0) {
      Event *prev_event = event_queue.back();
      next_event->eventCarryOver(*prev_event);
    }
    
    event_queue.push(next_event);    
      
    // Generate hits for this event
    int n_hits = hit_multiplicity_gen.Gaus(hit_mult_avg, hit_mult_stddev);

    for(int k = 0; k < n_hits; k++) {
      // Generate hits here..
      int rand_chip_id = hitgen.Integer(25000);
      int rand_x = hitgen.Integer(1024);
      int rand_y = hitgen.Integer(512);
      Hit h(rand_chip_id, rand_x, rand_y);


      //std::cout << "Generating hit number\t\t: " << k <<  std::endl;
      //hit_distribution->Fill(z_coord);

      //std::cout << "Z: " << z_coord << std::endl << "Theta: " << theta_angle << std::endl;

      // auto existing_hit = std::find(hit_vectors[event].begin(), hit_vectors[event].end(), h);
      // if(existing_hit != hit_vectors[event].end()) {
      //   // Discard if hit exists. Or maybe refresh counters?
      // } else {
      //   hit_vectors[event].push_back(h);
      // }
      next_event->addHit(h);
    }

    int carried_over_count = next_event->getCarriedOverCount();
    int not_carried_over_count = next_event->getNotCarriedOverCount();    

    std::cout << "Event " << event << ":\t @ " << time_vector[event] << " ns\t (t_delta = " << t_delta << " ns). \t";
    std::cout << carried_over_count << "/" << carried_over_count+not_carried_over_count << " hits carried over from previous event. ";
    std::cout << n_hits << " new hits. " <<  carried_over_count+n_hits << " hits in total." << std::endl;
  }
  
  TCanvas* c = new TCanvas();

  cnt_r_h->Draw();

  TCanvas* c_norm = new TCanvas();

  cnt_r_h->DrawNormalized();

  TCanvas* c_hit_dist = new TCanvas();

  hit_distribution->DrawNormalized();  

  TGraph *g = new TGraph(n_events, &x_vector[0], &time_vector[0]);
  TCanvas* c_graph = new TCanvas();
  g->Draw("AL");  


// Print summary
  cout << "Moments of Distribution:\n"  << " - Mean = " << cnt_r_h->GetMean() << " +- "
       << cnt_r_h->GetMeanError() << "\n" << " - RMS = " << cnt_r_h->GetRMS() << " +- "
       << cnt_r_h->GetRMSError() << "\n" << " - Skewness = " << cnt_r_h->GetSkewness() << "\n"
       << " - Kurtosis = " << cnt_r_h->GetKurtosis() << "\n";
}
