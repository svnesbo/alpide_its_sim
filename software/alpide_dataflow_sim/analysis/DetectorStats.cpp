/**
 * @file   DetectorStats.cpp
 * @author Simon Voigt Nesbo
 * @date   January 17, 2017
 * @brief  Statistics for one layer in ITS detector
 *
 */

#include "DetectorStats.hpp"
#include <iostream>
#include <fstream>
#include <tuple>
#include "TFile.h"
#include "TDirectory.h"
#include "TCanvas.h"
#include "TH2F.h"
#include "TH1F.h"
#include "THStack.h"
#include "misc.h"


///@brief Constructor for DetectorStats class.
///@param event_rate_khz Interaction event rate in kilohertz
///@param sim_time_ns Simulation time (in nanoseconds).
///                   Used for data rate calculations.
///@param sim_run_data_path Path to directory with simulation data.
DetectorStats::DetectorStats(ITS::detectorConfig config,
                             unsigned int event_rate_khz,
                             unsigned long sim_time_ns,
                             const char* sim_run_data_path)
  : mConfig(config)
  , mEventRateKhz(event_rate_khz)
  , mSimTimeNs(sim_time_ns)
  , mSimRunDataPath(sim_run_data_path)
{
  mNumLayers = 0;

  mLayerStats.resize(ITS::N_LAYERS, nullptr);

  for(unsigned int layer_num = 0; layer_num < 7; layer_num++) {
    if(config.layer[layer_num].num_staves > 0) {
      mLayerStats[layer_num] = new ITSLayerStats(layer_num,
                                                 config.layer[layer_num].num_staves,
                                                 sim_time_ns,
                                                 sim_run_data_path);
      mNumLayers++;
    }
  }
}


void DetectorStats::plotDetector(bool create_png, bool create_pdf)
{
  std::string root_filename = mSimRunDataPath + std::string("/busy_data.root");

  // Create/recreate root file
  TFile *f = new TFile(root_filename.c_str(), "recreate");

  // Todo: check that we have the same number of triggers in all RUs?
  uint64_t num_triggers = 0;

  //----------------------------------------------------------------------------
  // Generate plots for each layer included in the simulation
  //----------------------------------------------------------------------------
  for(unsigned int layer_num = 0; layer_num < ITS::N_LAYERS; layer_num++) {
    if(mLayerStats[layer_num] != nullptr) {
      mLayerStats[layer_num]->plotLayer(create_png, create_pdf);
      num_triggers = mLayerStats[layer_num]->getNumTriggers();
    }
  }


  //----------------------------------------------------------------------------
  // Save busy/busyv/flush/etc counts for each layer to CSV file
  //----------------------------------------------------------------------------
  std::string busy_count_filename = mSimRunDataPath + "/busy_count.csv";
  std::ofstream busy_count_file(busy_count_filename);
  if(!busy_count_file.is_open()) {
    std::cerr << "Error opening file " << busy_count_filename << std::endl;
    return;
  }

  busy_count_file << "Num_triggers; " << num_triggers << std::endl << std::endl;;
  busy_count_file << "Layer; BUSY; BUSYV; FLUSH; ABORT; FATAL" << std::endl;

  for(unsigned int layer = 0; layer < ITS::N_LAYERS; layer++) {
    if(mLayerStats[layer] != nullptr) {
      busy_count_file << layer << "; ";
      busy_count_file << mLayerStats[layer]->getNumBusyEvents() << "; ";
      busy_count_file << mLayerStats[layer]->getNumBusyVEvents() << "; ";
      busy_count_file << mLayerStats[layer]->getNumFlushEvents() << "; ";
      busy_count_file << mLayerStats[layer]->getNumAbortEvents() << "; ";
      busy_count_file << mLayerStats[layer]->getNumFatalEvents() << std::endl;
    }
  }

  busy_count_file.close();


  //----------------------------------------------------------------------------
  // Prepare to start plotting stuff for the whole detector
  //----------------------------------------------------------------------------
  TCanvas* c1 = new TCanvas();
  c1->cd();


  mTrigSentCoverage.resize(num_triggers);
  mTrigSentExclFilteringCoverage.resize(num_triggers);
  mTrigReadoutCoverage.resize(num_triggers);
  mTrigReadoutExclFilteringCoverage.resize(num_triggers);


  //----------------------------------------------------------------------------
  // Plot average trigger distribution and readout coverage vs. trigger
  //----------------------------------------------------------------------------
  TH1D *h1 = new TH1D("h_avg_trig_ctrl_link_efficiency_detector",
                      "Average Trigger Distribution Efficiency - Detector",
                      num_triggers,0,num_triggers);
  TH1D *h2 = new TH1D("h_avg_trig_ctrl_link_excl_filter_efficiency_detector",
                      "Average Trigger Distribution Efficiency Excluding Filtering - Detector",
                       num_triggers,0,num_triggers);
  TH1D *h3 = new TH1D("h_avg_trig_readout_efficiency_detector",
                       "Average Trigger Readout Efficiency - Detector",
                       num_triggers,0,num_triggers);
  TH1D *h4 = new TH1D("h_avg_trig_readout_excl_filter_efficiency_detector",
                       "Average Trigger Readout Efficiency Excluding Filtering - Detector",
                       num_triggers,0,num_triggers);

  for(uint64_t trigger_id = 0; trigger_id < num_triggers; trigger_id++) {
    double trig_sent_coverage = 0.0;
    double trig_sent_excl_filter_coverage = 0.0;
    double trig_readout_coverage = 0.0;
    double trig_readout_excl_filter_coverage = 0.0;
    for(unsigned int layer = 0; layer < ITS::N_LAYERS; layer++) {
      if(mLayerStats[layer] != nullptr) {
        trig_sent_coverage += mLayerStats[layer]->getTrigSentCoverage(trigger_id);
        trig_sent_excl_filter_coverage += mLayerStats[layer]->getTrigSentExclFilteringCoverage(trigger_id);
        trig_readout_coverage += mLayerStats[layer]->getTrigReadoutCoverage(trigger_id);
        trig_readout_excl_filter_coverage += mLayerStats[layer]->getTrigReadoutExclFilteringCoverage(trigger_id);
      }
    }

    mTrigSentCoverage[trigger_id] = trig_sent_coverage/mNumLayers;
    mTrigSentExclFilteringCoverage[trigger_id] = trig_sent_excl_filter_coverage/mNumLayers;
    mTrigReadoutCoverage[trigger_id] = trig_readout_coverage/mNumLayers;
    mTrigReadoutExclFilteringCoverage[trigger_id] = trig_readout_excl_filter_coverage/mNumLayers;

    h1->Fill(trigger_id, mTrigSentCoverage[trigger_id]);
    h2->Fill(trigger_id, mTrigSentExclFilteringCoverage[trigger_id]);
    h3->Fill(trigger_id, mTrigReadoutCoverage[trigger_id]);
    h4->Fill(trigger_id, mTrigReadoutExclFilteringCoverage[trigger_id]);

    std::cout << "Detector: trigger ID " << trigger_id;
    std::cout << " distribution coverage: " << mTrigSentCoverage[trigger_id] << std::endl;

    std::cout << "Detector: trigger ID " << trigger_id;
    std::cout << " distribution coverage (excluding filtering): ";
    std::cout << mTrigSentExclFilteringCoverage[trigger_id] << std::endl;

    std::cout << "Detector: trigger ID " << trigger_id;
    std::cout << " readout coverage: " << mTrigReadoutCoverage[trigger_id] << std::endl;

    std::cout << "Detector: trigger ID " << trigger_id;
    std::cout << " readout coverage (excluding filtering): ";

    std::cout << mTrigReadoutExclFilteringCoverage[trigger_id] << std::endl;
  }

  scale_eff_plot_y_range(h1);
  scale_eff_plot_y_range(h2);
  scale_eff_plot_y_range(h3);
  scale_eff_plot_y_range(h4);

  h1->GetYaxis()->SetTitle("Efficiency");
  h2->GetYaxis()->SetTitle("Efficiency");
  h3->GetYaxis()->SetTitle("Efficiency");
  h4->GetYaxis()->SetTitle("Efficiency");
  h1->GetXaxis()->SetTitle("Trigger ID");
  h2->GetXaxis()->SetTitle("Trigger ID");
  h3->GetXaxis()->SetTitle("Trigger ID");
  h4->GetXaxis()->SetTitle("Trigger ID");

  h1->SetStats(false);
  h2->SetStats(false);
  h3->SetStats(false);
  h4->SetStats(false);
  c1->Update();

  if(create_png) {
    h1->Draw();
    c1->Print(Form("%s/png/Detector_avg_trig_ctrl_link_efficiency.png", mSimRunDataPath.c_str()));

    h2->Draw();
    c1->Print(Form("%s/png/Detector_avg_trig_ctrl_link_excl_filter_efficiency.png", mSimRunDataPath.c_str()));

    h3->Draw();
    c1->Print(Form("%s/png/Detector_avg_trig_readout_efficiency.png", mSimRunDataPath.c_str()));

    h4->Draw();
    c1->Print(Form("%s/png/Detector_avg_trig_readout_excl_filter_efficiency.png", mSimRunDataPath.c_str()));
  }

  if(create_pdf) {
    h1->Draw();
    c1->Print(Form("%s/pdf/Detector_avg_trig_ctrl_link_efficiency.pdf", mSimRunDataPath.c_str()));

    h2->Draw();
    c1->Print(Form("%s/pdf/Detector_avg_trig_ctrl_link_excl_filter_efficiency.pdf", mSimRunDataPath.c_str()));

    h3->Draw();
    c1->Print(Form("%s/pdf/Detector_avg_trig_readout_efficiency.pdf", mSimRunDataPath.c_str()));

    h4->Draw();
    c1->Print(Form("%s/pdf/Detector_avg_trig_readout_excl_filter_efficiency.pdf", mSimRunDataPath.c_str()));
  }

  h1->Write();
  h2->Write();
  h3->Write();
  h4->Write();


  //----------------------------------------------------------------------------
  // Plot trigger distribution and readout efficiency vs. RU vs. trigger
  //----------------------------------------------------------------------------
  TH2D* h5 = new TH2D("h_trig_ctrl_link_efficiency_detector",
                      "Trigger Distribution Efficiency - Detector",
                      num_triggers,0,num_triggers-1,
                      ITS::N_LAYERS, -0.5, ITS::N_LAYERS-0.5);
  TH2D* h6 = new TH2D("h_trig_ctrl_link_excl_filter_efficiency_detector",
                      "Trigger Distribution Efficiency Excluding Filtering - Detector",
                      num_triggers,0,num_triggers-1,
                      ITS::N_LAYERS, -0.5, ITS::N_LAYERS-0.5);
  TH2D* h7 = new TH2D("h_trig_readout_efficiency_detector",
                      "Trigger Readout Efficiency - Detector",
                      num_triggers,0,num_triggers-1,
                      ITS::N_LAYERS, -0.5, ITS::N_LAYERS-0.5);
  TH2D* h8 = new TH2D("h_trig_readout_excl_filter_efficiency_detector",
                      "Trigger Readout Efficiency Excluding Filtering - Detector",
                      num_triggers,0,num_triggers-1,
                      ITS::N_LAYERS, -0.5, ITS::N_LAYERS-0.5);

  for(unsigned int layer = 0; layer < ITS::N_LAYERS; layer++) {
    if(mLayerStats[layer] != nullptr) {
      for(uint64_t trigger_id = 0; trigger_id < num_triggers; trigger_id++) {
        h5->Fill(trigger_id, layer, mLayerStats[layer]->getTrigSentCoverage(trigger_id));
        h6->Fill(trigger_id, layer, mLayerStats[layer]->getTrigSentExclFilteringCoverage(trigger_id));
        h7->Fill(trigger_id, layer, mLayerStats[layer]->getTrigReadoutCoverage(trigger_id));
        h8->Fill(trigger_id, layer, mLayerStats[layer]->getTrigReadoutExclFilteringCoverage(trigger_id));
      }
    }
  }

  scale_eff_plot_y_range(h5);
  scale_eff_plot_y_range(h6);
  scale_eff_plot_y_range(h7);
  scale_eff_plot_y_range(h8);

  h5->GetYaxis()->SetTitle("Layer number");
  h6->GetYaxis()->SetTitle("Layer number");
  h7->GetYaxis()->SetTitle("Layer number");
  h8->GetYaxis()->SetTitle("Layer number");
  h5->GetXaxis()->SetTitle("Trigger ID");
  h6->GetXaxis()->SetTitle("Trigger ID");
  h7->GetXaxis()->SetTitle("Trigger ID");
  h8->GetXaxis()->SetTitle("Trigger ID");

  h5->SetStats(false);
  h6->SetStats(false);
  h7->SetStats(false);
  h8->SetStats(false);

  h5->GetYaxis()->SetNdivisions(ITS::N_LAYERS);
  h6->GetYaxis()->SetNdivisions(ITS::N_LAYERS);
  h7->GetYaxis()->SetNdivisions(ITS::N_LAYERS);
  h8->GetYaxis()->SetNdivisions(ITS::N_LAYERS);


  if(create_png) {
    h5->Draw("COLZ");
    c1->Print(Form("%s/png/Detector_trig_ctrl_link_efficiency.png",
                   mSimRunDataPath.c_str()));

    h6->Draw("COLZ");
    c1->Print(Form("%s/png/Detector_trig_ctrl_link_excl_filter_efficiency.png",
                   mSimRunDataPath.c_str()));

    h7->Draw("COLZ");
    c1->Print(Form("%s/png/Detector_trig_readout_efficiency.png",
                   mSimRunDataPath.c_str()));

    h8->Draw("COLZ");
    c1->Print(Form("%s/png/Detector_trig_readout_excl_filter_efficiency.png",
                   mSimRunDataPath.c_str()));
  }

  if(create_pdf) {
    h5->Draw("COLZ");
    c1->Print(Form("%s/pdf/Detector_trig_ctrl_link_efficiency.pdf",
                   mSimRunDataPath.c_str()));

    h6->Draw("COLZ");
    c1->Print(Form("%s/pdf/Detector_trig_ctrl_link_excl_filter_efficiency.pdf",
                   mSimRunDataPath.c_str()));

    h7->Draw("COLZ");
    c1->Print(Form("%s/pdf/Detector_trig_readout_efficiency.pdf",
                   mSimRunDataPath.c_str()));

    h8->Draw("COLZ");
    c1->Print(Form("%s/pdf/Detector_trig_readout_excl_filter_efficiency.pdf",
                   mSimRunDataPath.c_str()));
  }

  h5->Write();
  h6->Write();
  h7->Write();
  h8->Write();



  //----------------------------------------------------------------------------
  // Plot busy, busyv, flush incompl, ro abort and fatal mode
  // link counts vs Layer number vs trigger ID
  //----------------------------------------------------------------------------
  TH2D* h9 = new TH2D("h_busy_link_count_map_detector",
                      "Busy Link Count - Detector",
                      num_triggers,0,num_triggers-1,
                      ITS::N_LAYERS, -0.5, ITS::N_LAYERS-0.5);
  TH2D* h10 = new TH2D("h_busyv_link_count_map_detector",
                      "Busy Violation Link Count - Detector",
                      num_triggers,0,num_triggers-1,
                       ITS::N_LAYERS, -0.5, ITS::N_LAYERS-0.5);
  TH2D* h11 = new TH2D("h_flush_link_count_map_detector",
                       "Flushed Incomplete Link Count - Detector",
                       num_triggers,0,num_triggers-1,
                       ITS::N_LAYERS, -0.5, ITS::N_LAYERS-0.5);
  TH2D* h12 = new TH2D("h_abort_link_count_map_detector",
                       "Readout Abort Link Count - Detector",
                       num_triggers,0,num_triggers-1,
                       ITS::N_LAYERS, -0.5, ITS::N_LAYERS-0.5);
  TH2D* h13 = new TH2D("h_fatal_link_count_map_detector",
                       "Fatal Mode Link Count - Detector",
                       num_triggers,0,num_triggers-1,
                       ITS::N_LAYERS, -0.5, ITS::N_LAYERS-0.5);

  for(unsigned int layer = 0; layer < ITS::N_LAYERS; layer++) {
    if(mLayerStats[layer] != nullptr) {
      std::vector<unsigned int> Layer_busy_link_count = mLayerStats[layer]->getBusyLinkCount();
      std::vector<unsigned int> Layer_busyv_link_count = mLayerStats[layer]->getBusyVLinkCount();
      std::vector<unsigned int> Layer_flush_link_count = mLayerStats[layer]->getFlushLinkCount();
      std::vector<unsigned int> Layer_abort_link_count = mLayerStats[layer]->getAbortLinkCount();
      std::vector<unsigned int> Layer_fatal_link_count = mLayerStats[layer]->getFatalLinkCount();

      if(Layer_busy_link_count.size() != num_triggers ||
         Layer_busyv_link_count.size() != num_triggers ||
         Layer_flush_link_count.size( )!= num_triggers ||
         Layer_abort_link_count.size( )!= num_triggers ||
         Layer_fatal_link_count.size( )!= num_triggers)
      {
        std::cout << "Error: Number of triggers in busy/busyv/flush/abort/fatal";
        std::cout << "link count vectors from Layer " << layer;
        std::cout << " does not matched expected number of triggers. " << std::endl;
        exit(-1);
      }

      for(uint64_t trigger_id = 0; trigger_id < num_triggers; trigger_id++) {
        h9->Fill(trigger_id, layer, Layer_busy_link_count[trigger_id]);
        h10->Fill(trigger_id, layer, Layer_busyv_link_count[trigger_id]);
        h11->Fill(trigger_id, layer, Layer_flush_link_count[trigger_id]);
        h12->Fill(trigger_id, layer, Layer_abort_link_count[trigger_id]);
        h13->Fill(trigger_id, layer, Layer_fatal_link_count[trigger_id]);
      }
    }
  }

  h9->GetYaxis()->SetTitle("Layer Number");
  h10->GetYaxis()->SetTitle("Layer Number");
  h11->GetYaxis()->SetTitle("Layer Number");
  h12->GetYaxis()->SetTitle("Layer Number");
  h13->GetYaxis()->SetTitle("Layer Number");
  h9->GetXaxis()->SetTitle("Trigger ID");
  h10->GetXaxis()->SetTitle("Trigger ID");
  h11->GetXaxis()->SetTitle("Trigger ID");
  h12->GetXaxis()->SetTitle("Trigger ID");
  h13->GetXaxis()->SetTitle("Trigger ID");

  h9->SetStats(false);
  h10->SetStats(false);
  h11->SetStats(false);
  h12->SetStats(false);
  h13->SetStats(false);

  h9->GetYaxis()->SetNdivisions(ITS::N_LAYERS);
  h10->GetYaxis()->SetNdivisions(ITS::N_LAYERS);
  h11->GetYaxis()->SetNdivisions(ITS::N_LAYERS);
  h12->GetYaxis()->SetNdivisions(ITS::N_LAYERS);
  h13->GetYaxis()->SetNdivisions(ITS::N_LAYERS);

  if(create_png) {
    h9->Draw("COLZ");
    c1->Print(Form("%s/png/Detector_busy_link_count_map.png", mSimRunDataPath.c_str()));

    h10->Draw("COLZ");
    c1->Print(Form("%s/png/Detector_busyv_link_count_map.png", mSimRunDataPath.c_str()));

    h11->Draw("COLZ");
    c1->Print(Form("%s/png/Detector_flush_link_count_map.png", mSimRunDataPath.c_str()));

    h12->Draw("COLZ");
    c1->Print(Form("%s/png/Detector_abort_link_count_map.png", mSimRunDataPath.c_str()));

    h13->Draw("COLZ");
    c1->Print(Form("%s/png/Detector_fatal_link_count_map.png", mSimRunDataPath.c_str()));
  }

  if(create_pdf) {
    h9->Draw("COLZ");
    c1->Print(Form("%s/pdf/Detector_busy_link_count_map.pdf", mSimRunDataPath.c_str()));

    h10->Draw("COLZ");
    c1->Print(Form("%s/pdf/Detector_busyv_link_count_map.pdf", mSimRunDataPath.c_str()));

    h11->Draw("COLZ");
    c1->Print(Form("%s/pdf/Detector_flush_link_count_map.pdf", mSimRunDataPath.c_str()));

    h12->Draw("COLZ");
    c1->Print(Form("%s/pdf/Detector_abort_link_count_map.pdf", mSimRunDataPath.c_str()));

    h13->Draw("COLZ");
    c1->Print(Form("%s/pdf/Detector_fatal_link_count_map.pdf", mSimRunDataPath.c_str()));
  }

  h9->Write();
  h10->Write();
  h11->Write();
  h12->Write();
  h13->Write();



  //----------------------------------------------------------------------------
  // Plot total number of busy, busyv, flush incompl, ro abort and fatal mode
  // event count vs layer
  //----------------------------------------------------------------------------
  TH1D *h14 = new TH1D("h_busy_vs_layer",
                       "Total busy event count vs layer",
                       ITS::N_LAYERS,-0.5,ITS::N_LAYERS-0.5);

  TH1D *h15 = new TH1D("h_busyv_vs_layer",
                       "Total busy violation event count vs layer",
                       ITS::N_LAYERS,-0.5,ITS::N_LAYERS-0.5);

  TH1D *h16 = new TH1D("h_flush_vs_layer",
                       "Total flushed incomplete event count vs layer",
                       ITS::N_LAYERS,-0.5,ITS::N_LAYERS-0.5);

  TH1D *h17 = new TH1D("h_abort_vs_layer",
                       "Total readout abort event count vs layer",
                       ITS::N_LAYERS,-0.5,ITS::N_LAYERS-0.5);

  TH1D *h18 = new TH1D("h_fatal_vs_layer",
                       "Total fatal mode event count vs layer",
                       ITS::N_LAYERS,-0.5,ITS::N_LAYERS-0.5);

  h14->GetXaxis()->SetTitle("Layer number");
  h15->GetXaxis()->SetTitle("Layer number");
  h16->GetXaxis()->SetTitle("Layer number");
  h17->GetXaxis()->SetTitle("Layer number");
  h18->GetXaxis()->SetTitle("Layer number");

  h14->GetYaxis()->SetTitle("Busy event count");
  h15->GetYaxis()->SetTitle("Busy violation event count");
  h16->GetYaxis()->SetTitle("Flushed incomplete event count");
  h17->GetYaxis()->SetTitle("Readout abort event count");
  h18->GetYaxis()->SetTitle("Fatal mode event count");

  for(unsigned int layer = 0; layer < ITS::N_LAYERS; layer++) {
    if(mLayerStats[layer] != nullptr) {
      h14->Fill(layer, mLayerStats[layer]->getNumBusyEvents());
      h15->Fill(layer, mLayerStats[layer]->getNumBusyVEvents());
      h16->Fill(layer, mLayerStats[layer]->getNumFlushEvents());
      h17->Fill(layer, mLayerStats[layer]->getNumAbortEvents());
      h18->Fill(layer, mLayerStats[layer]->getNumFatalEvents());
    }
  }

  h14->SetFillColor(33);
  h15->SetFillColor(33);
  h16->SetFillColor(33);
  h17->SetFillColor(33);
  h18->SetFillColor(33);
  h14->SetStats(false);
  h15->SetStats(false);
  h16->SetStats(false);
  h17->SetStats(false);
  h18->SetStats(false);
  c1->Update();

  if(create_png) {
    h14->Draw("BAR1");
    c1->Print(Form("%s/png/Detector_busy_event_count_vs_layer.png", mSimRunDataPath.c_str()));

    h15->Draw("BAR1");
    c1->Print(Form("%s/png/Detector_busyv_event_count_vs_layer.png", mSimRunDataPath.c_str()));

    h16->Draw("BAR1");
    c1->Print(Form("%s/png/Detector_flush_event_count_vs_layer.png", mSimRunDataPath.c_str()));

    h17->Draw("BAR1");
    c1->Print(Form("%s/png/Detector_abort_event_count_vs_layer.png", mSimRunDataPath.c_str()));

    h18->Draw("BAR1");
    c1->Print(Form("%s/png/Detector_fatal_event_count_vs_layer.png", mSimRunDataPath.c_str()));
  }

  if(create_pdf) {
    h14->Draw("BAR1");
    c1->Print(Form("%s/pdf/Detector_busy_event_count_vs_layer.pdf", mSimRunDataPath.c_str()));

    h15->Draw("BAR1");
    c1->Print(Form("%s/pdf/Detector_busyv_event_count_vs_layer.pdf", mSimRunDataPath.c_str()));

    h16->Draw("BAR1");
    c1->Print(Form("%s/pdf/Detector_flush_event_count_vs_layer.pdf", mSimRunDataPath.c_str()));

    h17->Draw("BAR1");
    c1->Print(Form("%s/pdf/Detector_abort_event_count_vs_layer.pdf", mSimRunDataPath.c_str()));

    h18->Draw("BAR1");
    c1->Print(Form("%s/pdf/Detector_fatal_event_count_vs_layer.pdf", mSimRunDataPath.c_str()));
  }

  h14->Write();
  h15->Write();
  h16->Write();
  h17->Write();
  h18->Write();



  //----------------------------------------------------------------------------
  // Plot average trigger distribution and readout efficiency vs layer
  //----------------------------------------------------------------------------
  TH1D *h19 = new TH1D("h_avg_trig_distr_efficiency_vs_layer",
                       "Average trigger distribution efficiency vs layer",
                       ITS::N_LAYERS,-0.5,ITS::N_LAYERS-0.5);

  TH1D *h20 = new TH1D("h_avg_readout_efficiency_vs_layer",
                       "Average readout efficiency vs layer",
                       ITS::N_LAYERS,-0.5,ITS::N_LAYERS-0.5);

  h19->GetXaxis()->SetTitle("Layer number");
  h19->GetYaxis()->SetTitle("Efficiency");

  h20->GetXaxis()->SetTitle("Layer number");
  h20->GetYaxis()->SetTitle("Efficiency");

  for(unsigned int layer = 0; layer < ITS::N_LAYERS; layer++) {
    if(mLayerStats[layer] != nullptr) {
      h19->Fill(layer, mLayerStats[layer]->getAvgTrigDistrEfficiency());
      h20->Fill(layer, mLayerStats[layer]->getAvgTrigReadoutEfficiency());
    }
  }

  scale_eff_plot_y_range(h19);
  scale_eff_plot_y_range(h20);

  h19->SetFillColor(33);
  h20->SetFillColor(33);
  h19->SetStats(false);
  h20->SetStats(false);
  c1->Update();

  if(create_png) {
    h19->Draw("BAR1");
    c1->Print(Form("%s/png/Detector_avg_trig_distr_efficiency_vs_layer.png", mSimRunDataPath.c_str()));

    h20->Draw("BAR1");
    c1->Print(Form("%s/png/Detector_avg_readout_efficiency_vs_layer.png", mSimRunDataPath.c_str()));
  }

  if(create_pdf) {
    h19->Draw("BAR1");
    c1->Print(Form("%s/pdf/Detector_avg_trig_distr_efficiency_vs_layer.pdf", mSimRunDataPath.c_str()));

    h20->Draw("BAR1");
    c1->Print(Form("%s/pdf/Detector_avg_readout_efficiency_vs_layer.pdf", mSimRunDataPath.c_str()));
  }

  h19->Write();
  h20->Write();


  //----------------------------------------------------------------------------
  // Plot data rates (data and protocol, separately and combined)
  // TODO: Plot standard deviation as error bars?
  //----------------------------------------------------------------------------
  TH1D *h21 = new TH1D("h_data_rates",
                       "Data",
                       ITS::N_LAYERS,-0.5,ITS::N_LAYERS-0.5);

  TH1D *h22 = new TH1D("h_protocol_rates",
                       "Protocol",
                       ITS::N_LAYERS,-0.5,ITS::N_LAYERS-0.5);

  h21->GetXaxis()->SetTitle("Layer number");
  h21->GetYaxis()->SetTitle("Data rate [Mpbs]");

  h22->GetXaxis()->SetTitle("Layer number");
  h22->GetYaxis()->SetTitle("Data rate [Mpbs]");

  for(unsigned int layer = 0; layer < ITS::N_LAYERS; layer++) {
    if(mLayerStats[layer] != nullptr) {
      auto data_rates = mLayerStats[layer]->getDataRatesMbps();
      unsigned int num_staves = data_rates.size();
      for(unsigned int stave = 0; stave < num_staves; stave++) {
        h21->Fill(layer, data_rates[stave]/num_staves);
        std::cout << "Layer " << layer << " data rate: " << data_rates[stave] << " Mbps" << std::endl;
      }

      auto protocol_rates = mLayerStats[layer]->getProtocolRatesMbps();
      num_staves = protocol_rates.size(); // Should really be same as data_rates.size()
      for(unsigned int stave = 0; stave < num_staves; stave++) {
        h22->Fill(layer, protocol_rates[stave]/num_staves);
        std::cout << "Layer " << layer << " protocol rate: " << protocol_rates[stave] << " Mbps" << std::endl;
      }
    }
  }

  h21->SetFillColor(34);
  h22->SetFillColor(33);
  h21->SetStats(false);
  h22->SetStats(false);
  c1->Update();

  THStack *hs1 = new THStack("hs_data_rates_vs_layer", "Average RU Data Rates vs Layer");
  hs1->Add(h21);
  hs1->Add(h22);

  if(create_png) {
    hs1->Draw("BAR1");
    hs1->GetXaxis()->SetTitle("Layer number");
    hs1->GetYaxis()->SetTitle("Data rate [Mpbs]");
    c1->BuildLegend();
    c1->Print(Form("%s/png/Detector_avg_data_rates_vs_layer.png", mSimRunDataPath.c_str()));
  }

  if(create_pdf) {
    hs1->Draw("BAR1");
    hs1->GetXaxis()->SetTitle("Layer number");
    hs1->GetYaxis()->SetTitle("Data rate [Mpbs]");
    c1->BuildLegend();
    c1->Print(Form("%s/png/Detector_avg_data_rates_vs_layer.pdf", mSimRunDataPath.c_str()));
  }

  h21->Write();
  h22->Write();





  TNamed event_rate("event_rate_khz", Form("%d", mEventRateKhz));
  event_rate.Write();

  delete h1;
  delete h2;
  delete h3;
  delete h4;
  delete h5;
  delete h6;
  delete h7;
  delete h8;
  delete h9;
  delete h10;
  delete h11;
  delete h12;
  delete h13;
  delete h14;
  delete h15;
  delete h16;
  delete h17;
  delete h18;
  delete h19;
  delete h20;
  delete c1;
  delete f;
}
