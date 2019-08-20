#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <TFile.h>
#include <TCanvas.h>
#include <TROOT.h>
#include <TH1F.h>
#include <TH1I.h>
#include <THStack.h>
#include "../src/Detector/ITS/ITSDetectorConfig.hpp"
#include "../src/Detector/PCT/PCTDetectorConfig.hpp"

#define MAX_SIMS 12

unsigned int colors[MAX_SIMS] = {kRed, kGreen, kBlue, kOrange, kMagenta, kPink,
                                 kSpring, kTeal, kAzure, kViolet, kCyan, kYellow};


void plot_efficiency(const std::map<unsigned int, TFile*> &root_files,
                     bool create_png, bool create_pdf)
{
  TCanvas* c1 = new TCanvas();
  c1->cd();

  THStack *hs_trig = new THStack("hs_trig","Average trigger distribution efficiency");
  THStack *hs_rdo = new THStack("hs_rdo","Average readout efficiency");

  // NOSTACKB is apparently not supported for histogram stacks in ROOT 5:
  // https://root-forum.cern.ch/t/thstack-draw-option-nostackb-not-working/22398
  // So I have to do all this trickery with some extra bins to have the bars
  // line up nicely next to each other
  unsigned int num_sims = root_files.size();
  unsigned int num_bins_per_layer = num_sims+2;
  unsigned int num_bins = num_bins_per_layer*ITS::N_LAYERS;

  if(num_sims > MAX_SIMS) {
    std::cerr << "Error: maximum 12 simulations" << std::endl;
    exit(-1);
  }

  std::vector<TH1D*> h_trig_eff_vec;
  std::vector<TH1D*> h_rdo_eff_vec;

  unsigned int sim_counter = 0;

  for(auto sim_it = root_files.begin(); sim_it != root_files.end(); sim_it++) {
    unsigned int sim_event_rate_khz = sim_it->first;
    std::string plot_title;

    if(sim_event_rate_khz >= 1000) {
      plot_title = std::to_string((double)sim_event_rate_khz/1000);
      // 2 digits after comma is fine
      plot_title = plot_title.substr(0,4);
      plot_title += " MHz";
    } else {
      plot_title = std::to_string(sim_event_rate_khz);
      plot_title += " kHz";
    }

    h_trig_eff_vec.push_back(new TH1D(Form("h_trigger_efficiency_%d", sim_counter),
                                      plot_title.c_str(),
                                      num_bins,-0.5,ITS::N_LAYERS-0.5));

    h_rdo_eff_vec.push_back(new TH1D(Form("h_readout_efficiency_%d", sim_counter),
                                     plot_title.c_str(),
                                     num_bins,-0.5,ITS::N_LAYERS-0.5));

    h_trig_eff_vec.back()->GetXaxis()->SetTitle("Layer number");
    h_trig_eff_vec.back()->GetYaxis()->SetTitle("Efficiency");

    h_rdo_eff_vec.back()->GetXaxis()->SetTitle("Layer number");
    h_rdo_eff_vec.back()->GetYaxis()->SetTitle("Efficiency");

    h_rdo_eff_vec.back()->SetFillColor(colors[sim_counter]);
    h_trig_eff_vec.back()->SetFillColor(colors[sim_counter]);

    unsigned int lay_start_bin = 2+sim_counter;
    unsigned int lay_bin_num = lay_start_bin;

    TH1D* h_sim_trig = nullptr;
    TH1D* h_sim_rdo  = nullptr;

    sim_it->second->GetObject("h_avg_trig_distr_efficiency_vs_layer", h_sim_trig);
    sim_it->second->GetObject("h_avg_readout_efficiency_vs_layer", h_sim_rdo);

    if(h_sim_trig == nullptr) {
      std::cerr << "Error retrieving h_avg_trig_distr_efficiency_vs_layer" << std::endl;
      std::cerr << "from root file for " << sim_it->first << " kHz." << std::endl;
      exit(-1);
    }

    if(h_sim_rdo == nullptr) {
      std::cerr << "Error retrieving h_avg_readout_efficiency_vs_layer" << std::endl;
      std::cerr << "from root file for " << sim_it->first << " kHz." << std::endl;
      exit(-1);
    }

    for(unsigned int layer = 0; layer < ITS::N_LAYERS; layer++) {
      std::cout << "Layer " << layer << " - " << sim_it->first << " kHz";
      std::cout << ". Trig eff: " << h_sim_trig->GetBinContent(layer+1);
      std::cout << ", RO eff:" << h_sim_rdo->GetBinContent(layer+1) << std::endl;

      // Skip underflow bin (bin 0) in h_sim_trig.
      h_trig_eff_vec.back()->SetBinContent(lay_bin_num, h_sim_trig->GetBinContent(layer+1));

      // Skip underflow bin (bin 0) in h_sim_rdo.
      h_rdo_eff_vec.back()->SetBinContent(lay_bin_num, h_sim_rdo->GetBinContent(layer+1));

      lay_bin_num += num_bins_per_layer;
    }

    sim_counter++;

    hs_trig->Add(h_trig_eff_vec.back());
    hs_rdo->Add(h_rdo_eff_vec.back());

    delete h_sim_trig;
    delete h_sim_rdo;
  }

  hs_trig->Draw("NOSTACKB");
  hs_trig->GetXaxis()->SetNdivisions(ITS::N_LAYERS);
  c1->BuildLegend();
  c1->Print("h_trigger_efficiency.png");

  hs_rdo->Draw("NOSTACKB");
  hs_rdo->GetXaxis()->SetNdivisions(ITS::N_LAYERS);
  c1->BuildLegend();
  c1->Print("h_readout_efficiency.png");


  // Clean up

  for(auto it = h_trig_eff_vec.begin(); it != h_trig_eff_vec.end(); it++)
    delete *it;

  for(auto it = h_rdo_eff_vec.begin(); it != h_rdo_eff_vec.end(); it++)
    delete *it;

  delete hs_trig;
  delete hs_rdo;
  delete c1;
}


void compare_simulations(const std::vector<std::string> &root_filenames,
                         bool create_png, bool create_pdf)
{
  std::cout << std::endl;
  std::cout << "Simulation files: " << std::endl;

  std::map<unsigned int, TFile*> root_files;

  for(auto it = root_filenames.begin(); it != root_filenames.end(); it++) {
    std::cout << *it << std::endl;
    TFile* root_file = TFile::Open(it->c_str());
    root_file->cd();
    TNamed* event_rate_obj = nullptr;
    root_file->GetObject("event_rate_khz", event_rate_obj);

    if(event_rate_obj == nullptr) {
      std::cerr << "Error: could not get event rate from file " << *it << std::endl;
      exit(-1);
    }

    unsigned int event_rate_khz = std::atol(event_rate_obj->GetTitle());

    std::cout << "Event rate: " << event_rate_khz << std::endl;

    if(root_files.find(event_rate_khz) != root_files.end()) {
      std::cerr << "Error: already loaded root file for ";
      std::cerr << event_rate_khz << " kHz." << std::endl;
      exit(-1);
    }

    root_files[event_rate_khz] = root_file;

    delete event_rate_obj;
  }

  //gROOT->SetBatch(kTRUE);

  plot_efficiency(root_files ,create_png, create_pdf);

  // Clean up
  for(auto it = root_files.begin(); it != root_files.end(); it++)
    delete it->second;
}


void print_help(void)
{
  std::cout << std::endl;
  std::cout << "compare_simulations:" << std::endl << std::endl;
  std::cout << "Takes the .root files generated by process_readout_trigger_stats, ";
  std::cout << "and compares the data for several simulations, and generates comparison plots.";
  std::cout << std::endl << std::endl;
  std::cout << "Usage:" << std::endl;
  std::cout << "compare_simulations [optional args] <1st root file> <2nd root file> ..." << std::endl;
  std::cout << std::endl;
  std::cout << "Optional arguments: " << std::endl;
  std::cout << "-h, --help: \tPrint this screen" << std::endl;
  std::cout << "-png, --png: \tWrite all plots to PNG files." << std::endl;
  std::cout << "-pdf, --pdf: \tWrite all plots to PDF files." << std::endl;
}



#define OPT_ARG_MAX 1

# ifndef __CINT__
int main(int argc, char** argv)
{
  std::string png_path = "";
  std::string pdf_path = "";

  bool create_png = false;
  bool create_pdf = false;

  if(argc == 1) {
    print_help();
    exit(0);
  }

  unsigned int opt_arg_count = 0;

  // Find optional arguments first
  for(unsigned int arg_num = 1; arg_num < argc && arg_num-1 < OPT_ARG_MAX; arg_num++) {
    if(strcmp(argv[arg_num], "-png") == 0 || strcmp(argv[arg_num], "--create_png") == 0) {
      create_png = true;
      opt_arg_count++;
    } else if(strcmp(argv[arg_num], "-pdf") == 0 || strcmp(argv[arg_num], "--create_pdf") == 0) {
      create_pdf = true;
      opt_arg_count++;
    } else if(strcmp(argv[arg_num], "-h") == 0 || strcmp(argv[arg_num], "--help") == 0) {
      opt_arg_count++;
      print_help();
      exit(0);
    } else if(strcmp(argv[arg_num], "-") == 0) {
      std::cout << "Unknown argument " << argv[arg_num] << std::endl;
      exit(0);
    }
    // Todo: make it possible to compare versus different variables, not just event rate
  }

  if(opt_arg_count+3 > argc) {
    std::cout << "Need at least two input files.." << std::endl;
    exit(0);
  }

  std::vector<std::string> root_files;

  // The rest of the arguments must be the list of root files
  for(unsigned int arg_num = 1+opt_arg_count; arg_num < argc; arg_num++) {
    root_files.push_back(std::string(argv[arg_num]));
  }

  compare_simulations(root_files, create_png, create_pdf);
  return 0;
}
# endif
