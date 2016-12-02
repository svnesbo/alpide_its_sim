#include <iostream>
#include <vector>
#include <algorithm>
#include <iterator>
#include <set>
#include "TGraph.h"
#include "TCanvas.h"
#include "TH1.h"
#include "TRandom3.h"

const int pixel_shaping_dead_time_ns = 200;
const int pixel_shaping_active_time_ns = 6000;

class Hit {
private:
  double z;
  double theta;
  int dead_time_counter;
  int active_time_counter;
public:
  Hit() {z = 0; theta = 0; dead_time_counter = 0; active_time_counter = 0; }
  Hit(double z_in, double theta_in, int dead_time_in = pixel_shaping_dead_time_ns, int active_time_in = pixel_shaping_active_time_ns) :
    z(z_in),
    theta(theta_in),
    dead_time_counter(dead_time_in),
    active_time_counter(active_time_in)
    {
    }
  Hit(const Hit& h) :
    z(h.z),
    theta(h.theta),
    dead_time_counter(h.dead_time_counter),
    active_time_counter(h.active_time_counter)
    {
    }    
  bool isActive(void) { return (dead_time_counter == 0 && active_time_counter > 0); }
  void decreaseTimers(int time_ns) {
    if(dead_time_counter > 0) {
      dead_time_counter -= time_ns;
      if(dead_time_counter < 0) {
        active_time_counter += dead_time_counter;
        dead_time_counter = 0;
      }
    } else {
      active_time_counter -= time_ns;
    }

    if(active_time_counter < 0)
      active_time_counter = 0;
  }
  int timeLeft(void) { return dead_time_counter + active_time_counter; }
  
  inline bool operator==(const Hit& rhs) {
    return (this->z == rhs.z) && (this->theta == rhs.theta);
  }
  Hit& operator=(const Hit& rhs) {
    this->z = rhs.z;
    this->theta = rhs.theta;
    this->dead_time_counter = rhs.dead_time_counter;
    this->active_time_counter = rhs.active_time_counter;
    return *this;
  }
  double getZ(void) {return z;}
  double getTheta(void) {return theta;}
};

// time_vector is placed on heap to avoid stack overflow.
const int n_events = 1000;
double time_vector[n_events];
double x_vector[n_events];
std::vector<Hit> hit_vectors[n_events];


//std::vector<double> time_vector;
//std::vector<int> x_vector;

void test(){
  TH1F* cnt_r_h = new TH1F("count_rate",
                           "Count Rate;N_{Counts};# occurencies",
                           100, // Number of Bins
                           0, // Lower X Boundary
                           0.00001); // Upper X Boundary

  TH1F* hit_distribution = new TH1F("hit_distribution",
                                    "Hit distribution;N_{Counts};# occurencies",
                                    100, // Number of Bins
                                    0, // Lower X Boundary
                                    20); // Upper X Boundary  

  const float mean_count=3.6;
  const double BC_period_ns = 25;
  const double hit_mult_avg = 1000;
  const double hit_mult_stddev = 300;

  int carried_over_count;
  int not_carried_over_count;

  Hit h1(10,-3, 0, 1000);
  Hit h2(20,-3);
  Hit h3(10,3);
  Hit h4(10,-3);
  Hit h5(1234,12);
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
    //cnt_r_h->Fill(t_delta);

    // Add hit to histogram
    //hit_distribution->Fill(hitgen.Integer(21));

    // Build event time vector
    if(event == 0)
//      time_vector.push_back(t_delta);
      time_vector[event] = t_delta;
    else
//      time_vector.push_back(time_vector.back() + t_delta);
      time_vector[event] = time_vector[event-1] + t_delta;

    //std::cout << "Event number\t\t: " << event << "\t\t Time: \t" << time_vector[event] << " ns." << std::endl;

//    x_vector.push_back(i);
    x_vector[event] = event;


    // Copy hits that "carry over" to this event because of the long shaping time
    //std::cout << "Copying hits from previous event.. " << std::endl;
    carried_over_count = 0;
    not_carried_over_count = 0;
    if(event > 0) {
      for(std::vector<Hit>::iterator it = hit_vectors[event-1].begin(); it != hit_vectors[event-1].end(); it++) {
        //std::cout << "Hit with coords z = " << it->getZ() << " and theta = " << it->getTheta();
        if(it->timeLeft() > t_delta) {
          //std::cout << " will carry over." << std::endl;
          Hit h = *it;
          //std::cout << "Decreasing timer.. " << std::endl;
          h.decreaseTimers(t_delta);
          //std::cout << "Storing new hit.. " << std::endl;          
          hit_vectors[event].push_back(h);
          carried_over_count++;
        } else {
          //std::cout << "Hit with coords z = " << it->getZ() << " and theta = " << it->getTheta();          
          //std::cout << " will not carry over." << std::endl;
          not_carried_over_count++;
        }
      }
    }
      
    // Generate hits for this event
    int n_hits = hit_multiplicity_gen.Gaus(hit_mult_avg, hit_mult_stddev);

    //std::cout << "Number of hits: " << n_hits << std::endl;


    // TODO: Is it faster to resize the vector to the size of n_hits,
    //       and then insert the hits at the right indexes instead of using push_back()?
    for(int k = 0; k < n_hits; k++) {
      // Generate hits here..
      int z_coord = hitgen.Integer(21);
      int theta_angle = hitgen.Integer(361);
      Hit h(z_coord, theta_angle);


      //std::cout << "Generating hit number\t\t: " << k <<  std::endl;
      //hit_distribution->Fill(z_coord);

      //std::cout << "Z: " << z_coord << std::endl << "Theta: " << theta_angle << std::endl;

      // auto existing_hit = std::find(hit_vectors[event].begin(), hit_vectors[event].end(), h);
      // if(existing_hit != hit_vectors[event].end()) {
      //   // Discard if hit exists. Or maybe refresh counters?
      // } else {
      //   hit_vectors[event].push_back(h);
      // }
      
      hit_vectors[event].push_back(h);      
    }

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
