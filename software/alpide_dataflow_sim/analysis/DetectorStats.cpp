/**
 * @file   DetectorStats.cpp
 * @author Simon Voigt Nesbo
 * @date   January 17, 2017
 * @brief  Statistics for one layer in ITS detector
 *
 */

#include "DetectorStats.hpp"
#include <iostream>
#include <tuple>
#include "TFile.h"
#include "TDirectory.h"
#include "TCanvas.h"
#include "TH2F.h"
#include "TH1F.h"
#include "THStack.h"


DetectorStats::DetectorStats(ITS::detectorConfig config,
                             unsigned int event_rate_khz,
                             const char* sim_run_data_path)
  : mConfig(config)
  , mEventRateKhz(event_rate_khz)
  , mSimRunDataPath(sim_run_data_path)
{
  mNumLayers = 0;

  mLayerStats.resize(ITS::N_LAYERS, nullptr);

  for(unsigned int layer_num = 0; layer_num < 7; layer_num++) {
    if(config.layer[layer_num].num_staves > 0) {
      mLayerStats[layer_num] = new ITSLayerStats(layer_num,
                                                 config.layer[layer_num].num_staves,
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


  TCanvas* c1 = new TCanvas();
  c1->cd();


  mTrigSentCoverage.resize(num_triggers);
  mTrigSentExclFilteringCoverage.resize(num_triggers);
  mTrigReadoutCoverage.resize(num_triggers);
  mTrigReadoutExclFilteringCoverage.resize(num_triggers);


  //----------------------------------------------------------------------------
  // Plot average trigger distribution and readout coverage vs. trigger
  //----------------------------------------------------------------------------
  TH1D *h1 = new TH1D("h_avg_trig_ctrl_link_coverage_detector",
                      "Average Trigger Distribution Coverage - Detector",
                      num_triggers,0,num_triggers-1);
  TH1D *h2 = new TH1D("h_avg_trig_ctrl_link_excl_filter_coverage_detector",
                      "Average Trigger Distribution Coverage Excluding Filtering - Detector",
                       num_triggers,0,num_triggers-1);
  TH1D *h3 = new TH1D("h_avg_trig_readout_coverage_detector",
                       "Average Trigger Readout Coverage - Detector",
                       num_triggers,0,num_triggers-1);
  TH1D *h4 = new TH1D("h_avg_trig_readout_excl_filter_coverage_detector",
                       "Average Trigger Readout Coverage Excluding Filtering - Detector",
                       num_triggers,0,num_triggers-1);

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

  h1->GetYaxis()->SetTitle("Coverage");
  h2->GetYaxis()->SetTitle("Coverage");
  h3->GetYaxis()->SetTitle("Coverage");
  h4->GetYaxis()->SetTitle("Coverage");
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
    c1->Print(Form("%s/png/Detector_avg_trig_ctrl_link_coverage.png", mSimRunDataPath.c_str()));

    h2->Draw();
    c1->Print(Form("%s/png/Detector_avg_trig_ctrl_link_excl_filter_coverage.png", mSimRunDataPath.c_str()));

    h3->Draw();
    c1->Print(Form("%s/png/Detector_avg_trig_readout_coverage.png", mSimRunDataPath.c_str()));

    h4->Draw();
    c1->Print(Form("%s/png/Detector_avg_trig_readout_excl_filter_coverage.png", mSimRunDataPath.c_str()));
  }

  if(create_pdf) {
    h1->Draw();
    c1->Print(Form("%s/pdf/Detector_avg_trig_ctrl_link_coverage.pdf", mSimRunDataPath.c_str()));

    h2->Draw();
    c1->Print(Form("%s/pdf/Detector_avg_trig_ctrl_link_excl_filter_coverage.pdf", mSimRunDataPath.c_str()));

    h3->Draw();
    c1->Print(Form("%s/pdf/Detector_avg_trig_readout_coverage.pdf", mSimRunDataPath.c_str()));

    h4->Draw();
    c1->Print(Form("%s/pdf/Detector_avg_trig_readout_excl_filter_coverage.pdf", mSimRunDataPath.c_str()));
  }

  h1->Write();
  h2->Write();
  h3->Write();
  h4->Write();


  //----------------------------------------------------------------------------
  // Plot trigger distribution and readout coverage vs. RU vs. trigger
  //----------------------------------------------------------------------------
  TH2D* h5 = new TH2D("h_trig_ctrl_link_coverage_detector",
                      "Trigger Distribution Coverage - Detector",
                      num_triggers,0,num_triggers-1,
                      ITS::N_LAYERS, -0.5, ITS::N_LAYERS-0.5);
  TH2D* h6 = new TH2D("h_trig_ctrl_link_excl_filter_coverage_detector",
                      "Trigger Distribution Coverage Excluding Filtering - Detector",
                      num_triggers,0,num_triggers-1,
                      ITS::N_LAYERS, -0.5, ITS::N_LAYERS-0.5);
  TH2D* h7 = new TH2D("h_trig_readout_coverage_detector",
                      "Trigger Readout Coverage - Detector",
                      num_triggers,0,num_triggers-1,
                      ITS::N_LAYERS, -0.5, ITS::N_LAYERS-0.5);
  TH2D* h8 = new TH2D("h_trig_readout_excl_filter_coverage_detector",
                      "Trigger Readout Coverage Excluding Filtering - Detector",
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

  h5->GetYaxis()->SetTitle("Layer number");
  h6->GetYaxis()->SetTitle("Layer number");
  h7->GetYaxis()->SetTitle("Layer number");
  h8->GetYaxis()->SetTitle("Layer number");
  h5->GetXaxis()->SetTitle("Trigger ID");
  h6->GetXaxis()->SetTitle("Trigger ID");
  h7->GetXaxis()->SetTitle("Trigger ID");
  h8->GetXaxis()->SetTitle("Trigger ID");

  h5->GetYaxis()->SetNdivisions(ITS::N_LAYERS);
  h6->GetYaxis()->SetNdivisions(ITS::N_LAYERS);
  h7->GetYaxis()->SetNdivisions(ITS::N_LAYERS);
  h8->GetYaxis()->SetNdivisions(ITS::N_LAYERS);


  if(create_png) {
    h5->Draw("COLZ");
    c1->Print(Form("%s/png/Detector_trig_ctrl_link_coverage.png",
                   mSimRunDataPath.c_str()));

    h6->Draw("COLZ");
    c1->Print(Form("%s/png/Detector_trig_ctrl_link_excl_filter_coverage.png",
                   mSimRunDataPath.c_str()));

    h7->Draw("COLZ");
    c1->Print(Form("%s/png/Detector_trig_readout_coverage.png",
                   mSimRunDataPath.c_str()));

    h8->Draw("COLZ");
    c1->Print(Form("%s/png/Detector_trig_readout_excl_filter_coverage.png",
                   mSimRunDataPath.c_str()));
  }

  if(create_pdf) {
    h5->Draw("COLZ");
    c1->Print(Form("%s/pdf/Detector_trig_ctrl_link_coverage.pdf",
                   mSimRunDataPath.c_str()));

    h6->Draw("COLZ");
    c1->Print(Form("%s/pdf/Detector_trig_ctrl_link_excl_filter_coverage.pdf",
                   mSimRunDataPath.c_str()));

    h7->Draw("COLZ");
    c1->Print(Form("%s/pdf/Detector_trig_readout_coverage.pdf",
                   mSimRunDataPath.c_str()));

    h8->Draw("COLZ");
    c1->Print(Form("%s/pdf/Detector_trig_readout_excl_filter_coverage.pdf",
                   mSimRunDataPath.c_str()));
  }

  h5->Write();
  h6->Write();
  h7->Write();
  h8->Write();



  //----------------------------------------------------------------------------
  // Plot busy and busy violation link counts vs Layer number vs trigger ID
  //----------------------------------------------------------------------------
  TH2D* h9 = new TH2D("h_busy_link_count_map_detector",
                      "Busy Link Count - Detector",
                      num_triggers,0,num_triggers-1,
                      ITS::N_LAYERS, -0.5, ITS::N_LAYERS-0.5);
  TH2D* h10 = new TH2D("h_busyv_link_count_map_detector",
                      "Busy Violation Link Count - Detector",
                      num_triggers,0,num_triggers-1,
                       ITS::N_LAYERS, -0.5, ITS::N_LAYERS-0.5);

  for(unsigned int layer = 0; layer < ITS::N_LAYERS; layer++) {
    if(mLayerStats[layer] != nullptr) {
      std::vector<unsigned int> Layer_busy_link_count = mLayerStats[layer]->getBusyLinkCount();
      std::vector<unsigned int> Layer_busyv_link_count = mLayerStats[layer]->getBusyVLinkCount();

      if(Layer_busy_link_count.size() != num_triggers || Layer_busyv_link_count.size() != num_triggers) {
        std::cout << "Error: Number of triggers in busy/busyv link count vectors from Layer " << layer;
        std::cout << " does not matched expected number of triggers. " << std::endl;
        exit(-1);
      }

      for(uint64_t trigger_id = 0; trigger_id < num_triggers; trigger_id++) {
        h9->Fill(trigger_id, layer, Layer_busy_link_count[trigger_id]);
        h10->Fill(trigger_id, layer, Layer_busyv_link_count[trigger_id]);
      }
    }
  }

  h9->GetYaxis()->SetTitle("Layer Number");
  h10->GetYaxis()->SetTitle("Layer Number");
  h9->GetXaxis()->SetTitle("Trigger ID");
  h10->GetXaxis()->SetTitle("Trigger ID");

  h9->GetYaxis()->SetNdivisions(ITS::N_LAYERS);
  h10->GetYaxis()->SetNdivisions(ITS::N_LAYERS);

  if(create_png) {
    h9->Draw("COLZ");
    c1->Print(Form("%s/png/Detector_busy_link_count_map.png", mSimRunDataPath.c_str()));

    h10->Draw("COLZ");
    c1->Print(Form("%s/png/Detector_busyv_link_count_map.png", mSimRunDataPath.c_str()));
  }

  if(create_pdf) {
    h9->Draw("COLZ");
    c1->Print(Form("%s/pdf/Detector_busy_link_count_map.pdf", mSimRunDataPath.c_str()));

    h10->Draw("COLZ");
    c1->Print(Form("%s/pdf/Detector_busyv_link_count_map.pdf", mSimRunDataPath.c_str()));
  }

  h9->Write();
  h10->Write();



  //----------------------------------------------------------------------------
  // Plot total number of busy and busy violation event count vs layer
  //----------------------------------------------------------------------------
  TH1D *h11 = new TH1D("h_busy_vs_layer",
                       "Total busy event count vs layer",
                       ITS::N_LAYERS,-0.5,ITS::N_LAYERS-0.5);

  TH1D *h12 = new TH1D("h_busyv_vs_layer",
                       "Total busy violation event count vs layer",
                       ITS::N_LAYERS,-0.5,ITS::N_LAYERS-0.5);

  h11->GetXaxis()->SetTitle("Layer number");
  h11->GetYaxis()->SetTitle("Busy event count");

  h12->GetXaxis()->SetTitle("Layer number");
  h12->GetYaxis()->SetTitle("Busy violation event count");

  for(unsigned int layer = 0; layer < ITS::N_LAYERS; layer++) {
    if(mLayerStats[layer] != nullptr) {
      h11->Fill(layer, mLayerStats[layer]->getNumBusyEvents());
      h12->Fill(layer, mLayerStats[layer]->getNumBusyVEvents());
    }
  }

  if(create_png) {
    h11->Draw();
    c1->Print(Form("%s/png/Detector_busy_event_count_vs_layer.png", mSimRunDataPath.c_str()));

    h12->Draw();
    c1->Print(Form("%s/png/Detector_busyv_event_count_vs_layer.png", mSimRunDataPath.c_str()));
  }

  if(create_pdf) {
    h11->Draw();
    c1->Print(Form("%s/pdf/Detector_busy_event_count_vs_layer.pdf", mSimRunDataPath.c_str()));

    h12->Draw();
    c1->Print(Form("%s/pdf/Detector_busyv_event_count_vs_layer.pdf", mSimRunDataPath.c_str()));
  }

  h11->Write();
  h12->Write();



  //----------------------------------------------------------------------------
  // Plot average trigger distribution and readout efficiency vs layer
  //----------------------------------------------------------------------------
  TH1D *h13 = new TH1D("h_avg_trig_distr_efficiency_vs_layer",
                       "Average trigger distribution efficiency vs layer",
                       ITS::N_LAYERS,-0.5,ITS::N_LAYERS-0.5);

  TH1D *h14 = new TH1D("h_avg_readout_efficiency_vs_layer",
                       "Average readout efficiency vs layer",
                       ITS::N_LAYERS,-0.5,ITS::N_LAYERS-0.5);

  h13->GetXaxis()->SetTitle("Layer number");
  h13->GetYaxis()->SetTitle("Efficiency");

  h14->GetXaxis()->SetTitle("Layer number");
  h14->GetYaxis()->SetTitle("Efficiency");

  for(unsigned int layer = 0; layer < ITS::N_LAYERS; layer++) {
    if(mLayerStats[layer] != nullptr) {
      h13->Fill(layer, mLayerStats[layer]->getAvgTrigDistrEfficiency());
      h14->Fill(layer, mLayerStats[layer]->getAvgTrigReadoutEfficiency());
    }
  }

  if(create_png) {
    h13->Draw();
    c1->Print(Form("%s/png/Detector_avg_trig_distr_efficiency_vs_layer.png", mSimRunDataPath.c_str()));

    h14->Draw();
    c1->Print(Form("%s/png/Detector_avg_readout_efficiency_vs_layer.png", mSimRunDataPath.c_str()));
  }

  if(create_pdf) {
    h13->Draw();
    c1->Print(Form("%s/pdf/Detector_avg_trig_distr_efficiency_vs_layer.pdf", mSimRunDataPath.c_str()));

    h14->Draw();
    c1->Print(Form("%s/pdf/Detector_avg_readout_efficiency_vs_layer.pdf", mSimRunDataPath.c_str()));
  }

  h13->Write();
  h14->Write();



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
  delete c1;
  delete f;
}
