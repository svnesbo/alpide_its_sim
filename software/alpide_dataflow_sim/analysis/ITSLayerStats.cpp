/**
 * @file   ITSLayerStats.cpp
 * @author Simon Voigt Nesbo
 * @date   November 20, 2017
 * @brief  Statistics for one layer in ITS detector
 *
 */

#include "ITSLayerStats.hpp"
#include "Detector/PCT/PCT_constants.hpp"
#include <iostream>
#include <cmath>
#include "TFile.h"
#include "TDirectory.h"
#include "TCanvas.h"
#include "TH2F.h"
#include "TH1F.h"
#include "THStack.h"
#include "TArray.h"
#include "misc.h"


///@brief Constructor for ITSLayerStats
///@param layer_num ITS Layer number
///@param num_staves Number of staves simulated in this layer
///@param sim_time_ns Simulation time (in nanoseconds).
///       Used for data rate calculations.
///@param sim_type "pct" or "its"
///@param path Path to simulation data directory
ITSLayerStats::ITSLayerStats(unsigned int layer_num, unsigned int num_staves,
                             unsigned long sim_time_ns, std::string sim_type,
                             const char* path)
  : mLayer(layer_num)
  , mNumStaves(num_staves)
  , mSimTimeNs(sim_time_ns)
  , mSimDataPath(path)
  , mNumTriggers(0)
{
  if(sim_type == "pct") {
    // Several staves per RU for pCT
    unsigned int num_staves_per_readout_unit = PCT::STAVES_PER_LAYER/PCT::READOUT_UNITS_PER_LAYER;
    mNumReadoutUnits = ceil(num_staves/(double)num_staves_per_readout_unit);
  } else if (sim_type == "its")
  {
    // Only 1 stave per RU for ITS, regardless of layer
    mNumReadoutUnits = num_staves;
  } else {
    std::cout << "Error: Unknown simulation type \"" << sim_type << "\"" << std::endl;
    exit(-1);
  }

  // Create and parse RU data
  for(unsigned int RU_num = 0; RU_num < mNumReadoutUnits; RU_num++) {
    mRUStats.emplace_back(layer_num, RU_num, sim_time_ns, path);
    mProtocolRatesMbps.push_back(mRUStats.back().getProtocolRateMbps());
    mDataRatesMbps.push_back(mRUStats.back().getDataRateMbps());
  }
}


void ITSLayerStats::plotLayer(bool create_png, bool create_pdf)
{
  TDirectory* current_dir = gDirectory;

  if(gDirectory == nullptr) {
    std::cout << "ITSLayerStats::plotLayer() error: gDirectory not initialized." << std::endl;
    exit(-1);
  }

  gDirectory->mkdir(Form("Layer_%i", mLayer));

  // Create and parse RU data, and generate plots in TFile
  for(unsigned int RU_num = 0; RU_num < mNumReadoutUnits; RU_num++) {
    // Keep changing back to this layer's directory,
    // because the plotRU() function changes the current directory.
    current_dir->cd(Form("Layer_%i", mLayer));
    mRUStats[RU_num].plotRU(create_png, create_pdf);
  }

  current_dir->cd(Form("Layer_%i", mLayer));


  TCanvas* c1 = new TCanvas();
  c1->cd();


  // Todo: check that we have the same number of triggers in all RUs?
  mNumTriggers = mRUStats[0].getNumTriggers();

  mTrigSentCoverage.resize(mNumTriggers);
  mTrigSentExclFilteringCoverage.resize(mNumTriggers);
  mTrigReadoutCoverage.resize(mNumTriggers);
  mTrigReadoutExclFilteringCoverage.resize(mNumTriggers);


  //----------------------------------------------------------------------------
  // Plot average trigger distribution and readout coverage vs. trigger
  //----------------------------------------------------------------------------
  TH1D *h1 = new TH1D("h_avg_trig_ctrl_link_efficiency",
                      Form("Average Trigger Distribution Efficiency - Layer %i", mLayer),
                      mNumTriggers,0,mNumTriggers);
  TH1D *h2 = new TH1D("h_avg_trig_ctrl_link_excl_filter_efficiency",
                       Form("Average Trigger Distribution Efficiency Excluding Filtering - Layer %i", mLayer),
                       mNumTriggers,0,mNumTriggers);
  TH1D *h3 = new TH1D("h_avg_trig_readout_efficiency",
                       Form("Average Trigger Readout Efficiency - Layer %i", mLayer),
                       mNumTriggers,0,mNumTriggers);
  TH1D *h4 = new TH1D("h_avg_trig_readout_excl_filter_efficiency",
                       Form("Average Trigger Readout Efficiency Excluding Filtering - Layer %i", mLayer),
                       mNumTriggers,0,mNumTriggers);

  for(uint64_t trigger_id = 0; trigger_id < mNumTriggers; trigger_id++) {
    double trig_sent_coverage = 0.0;
    double trig_sent_excl_filter_coverage = 0.0;
    double trig_readout_coverage = 0.0;
    double trig_readout_excl_filter_coverage = 0.0;
    for(unsigned int RU_num = 0; RU_num < mNumReadoutUnits; RU_num++) {
      trig_sent_coverage += mRUStats[RU_num].getTrigSentCoverage(trigger_id);
      trig_sent_excl_filter_coverage += mRUStats[RU_num].getTrigSentExclFilteringCoverage(trigger_id);
      trig_readout_coverage += mRUStats[RU_num].getTrigReadoutCoverage(trigger_id);
      trig_readout_excl_filter_coverage += mRUStats[RU_num].getTrigReadoutExclFilteringCoverage(trigger_id);
    }

    mTrigSentCoverage[trigger_id] = trig_sent_coverage/mNumReadoutUnits;
    mTrigSentExclFilteringCoverage[trigger_id] = trig_sent_excl_filter_coverage/mNumReadoutUnits;
    mTrigReadoutCoverage[trigger_id] = trig_readout_coverage/mNumReadoutUnits;
    mTrigReadoutExclFilteringCoverage[trigger_id] = trig_readout_excl_filter_coverage/mNumReadoutUnits;

    mAvgTrigDistrEfficiency += mTrigSentExclFilteringCoverage[trigger_id];
    mAvgTrigReadoutEfficiency += mTrigReadoutExclFilteringCoverage[trigger_id];

    h1->Fill(trigger_id, mTrigSentCoverage[trigger_id]);
    h2->Fill(trigger_id, mTrigSentExclFilteringCoverage[trigger_id]);
    h3->Fill(trigger_id, mTrigReadoutCoverage[trigger_id]);
    h4->Fill(trigger_id, mTrigReadoutExclFilteringCoverage[trigger_id]);

    std::cout << "Layer " << mLayer << ", trigger ID " << trigger_id;
    std::cout << " distribution coverage: " << mTrigSentCoverage[trigger_id] << std::endl;

    std::cout << "Layer " << mLayer << ", trigger ID " << trigger_id;
    std::cout << " distribution coverage (excluding filtering): ";
    std::cout << mTrigSentExclFilteringCoverage[trigger_id] << std::endl;

    std::cout << "Layer " << mLayer << ", trigger ID " << trigger_id;
    std::cout << " readout coverage: " << mTrigReadoutCoverage[trigger_id] << std::endl;

    std::cout << "Layer " << mLayer << ", trigger ID " << trigger_id;
    std::cout << " readout coverage (excluding filtering): ";
    std::cout << mTrigReadoutExclFilteringCoverage[trigger_id] << std::endl;
  }

  mAvgTrigDistrEfficiency /= mNumTriggers;
  mAvgTrigReadoutEfficiency /= mNumTriggers;

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
    c1->Print(Form("%s/png/Layer_%i_avg_trig_ctrl_link_efficiency.png",
                   mSimDataPath.c_str(), mLayer));

    h2->Draw();
    c1->Print(Form("%s/png/Layer_%i_avg_trig_ctrl_link_excl_filter_efficiency.png",
                   mSimDataPath.c_str(), mLayer));

    h3->Draw();
    c1->Print(Form("%s/png/Layer_%i_avg_trig_readout_efficiency.png",
                   mSimDataPath.c_str(), mLayer));

    h4->Draw();
    c1->Print(Form("%s/png/Layer_%i_avg_trig_readout_excl_filter_efficiency.png",
                   mSimDataPath.c_str(), mLayer));
  }

  if(create_pdf) {
    h1->Draw();
    c1->Print(Form("%s/pdf/Layer_%i_avg_trig_ctrl_link_efficiency.pdf",
                   mSimDataPath.c_str(), mLayer));

    h2->Draw();
    c1->Print(Form("%s/pdf/Layer_%i_avg_trig_ctrl_link_excl_filter_efficiency.pdf",
                   mSimDataPath.c_str(), mLayer));

    h3->Draw();
    c1->Print(Form("%s/pdf/Layer_%i_avg_trig_readout_efficiency.pdf",
                   mSimDataPath.c_str(), mLayer));

    h4->Draw();
    c1->Print(Form("%s/pdf/Layer_%i_avg_trig_readout_excl_filter_efficiency.pdf",
                   mSimDataPath.c_str(), mLayer));
  }

  h1->Write();
  h2->Write();
  h3->Write();
  h4->Write();


  //----------------------------------------------------------------------------
  // Plot trigger distribution and readout efficiency vs. RU vs. trigger
  //----------------------------------------------------------------------------
  TH2D* h5 = new TH2D(Form("h_trig_ctrl_link_efficiency_layer_%i", mLayer),
                      Form("Trigger Distribution Efficiency - Layer %i",
                           mLayer),
                      mNumTriggers,0,mNumTriggers,
                      mNumReadoutUnits, -0.5, mNumReadoutUnits-0.5);
  TH2D* h6 = new TH2D(Form("h_trig_ctrl_link_excl_filter_efficiency_layer_%i", mLayer),
                      Form("Trigger Distribution Efficiency Excluding Filtering - Layer %i",
                           mLayer),
                      mNumTriggers,0,mNumTriggers,
                      mNumReadoutUnits, -0.5, mNumReadoutUnits-0.5);
  TH2D* h7 = new TH2D(Form("h_trig_readout_efficiency_layer_%i", mLayer),
                      Form("Trigger Readout Efficiency - Layer %i", mLayer),
                      mNumTriggers,0,mNumTriggers,
                      mNumReadoutUnits, -0.5, mNumReadoutUnits-0.5);
  TH2D* h8 = new TH2D(Form("h_trig_readout_excl_filter_efficiency_layer_%i", mLayer),
                      Form("Trigger Readout Efficiency Excluding Filtering - Layer %i",
                           mLayer),
                      mNumTriggers,0,mNumTriggers,
                      mNumReadoutUnits, -0.5, mNumReadoutUnits-0.5);

  for(unsigned int RU_num = 0; RU_num < mNumReadoutUnits; RU_num++) {
    for(uint64_t trigger_id = 0; trigger_id < mNumTriggers; trigger_id++) {
      h5->Fill(trigger_id, RU_num, mRUStats[RU_num].getTrigSentCoverage(trigger_id));
      h6->Fill(trigger_id, RU_num, mRUStats[RU_num].getTrigSentExclFilteringCoverage(trigger_id));
      h7->Fill(trigger_id, RU_num, mRUStats[RU_num].getTrigReadoutCoverage(trigger_id));
      h8->Fill(trigger_id, RU_num, mRUStats[RU_num].getTrigReadoutExclFilteringCoverage(trigger_id));
    }
  }

  h5->GetYaxis()->SetTitle("RU Number");
  h6->GetYaxis()->SetTitle("RU Number");
  h7->GetYaxis()->SetTitle("RU Number");
  h8->GetYaxis()->SetTitle("RU Number");
  h5->GetXaxis()->SetTitle("Trigger ID");
  h6->GetXaxis()->SetTitle("Trigger ID");
  h7->GetXaxis()->SetTitle("Trigger ID");
  h8->GetXaxis()->SetTitle("Trigger ID");

  h5->SetStats(false);
  h6->SetStats(false);
  h7->SetStats(false);
  h8->SetStats(false);

  h5->GetYaxis()->SetNdivisions(mNumReadoutUnits);
  h6->GetYaxis()->SetNdivisions(mNumReadoutUnits);
  h7->GetYaxis()->SetNdivisions(mNumReadoutUnits);
  h8->GetYaxis()->SetNdivisions(mNumReadoutUnits);


  if(create_png) {
    h5->Draw("COLZ");
    c1->Print(Form("%s/png/Layer_%i_trig_ctrl_link_efficiency.png",
                   mSimDataPath.c_str(), mLayer));

    h6->Draw("COLZ");
    c1->Print(Form("%s/png/Layer_%i_trig_ctrl_link_excl_filter_efficiency.png",
                   mSimDataPath.c_str(), mLayer));

    h7->Draw("COLZ");
    c1->Print(Form("%s/png/Layer_%i_trig_readout_efficiency.png",
                   mSimDataPath.c_str(), mLayer));

    h8->Draw("COLZ");
    c1->Print(Form("%s/png/Layer_%i_trig_readout_excl_filter_efficiency.png",
                   mSimDataPath.c_str(), mLayer));
  }

  if(create_pdf) {
    h5->Draw("COLZ");
    c1->Print(Form("%s/pdf/Layer_%i_trig_ctrl_link_efficiency.pdf",
                   mSimDataPath.c_str(), mLayer));

    h6->Draw("COLZ");
    c1->Print(Form("%s/pdf/Layer_%i_trig_ctrl_link_excl_filter_efficiency.pdf",
                   mSimDataPath.c_str(), mLayer));

    h7->Draw("COLZ");
    c1->Print(Form("%s/pdf/Layer_%i_trig_readout_efficiency.pdf",
                   mSimDataPath.c_str(), mLayer));

    h8->Draw("COLZ");
    c1->Print(Form("%s/pdf/Layer_%i_trig_readout_excl_filter_efficiency.pdf",
                   mSimDataPath.c_str(), mLayer));
  }

  h5->Write();
  h6->Write();
  h7->Write();
  h8->Write();



  //----------------------------------------------------------------------------
  // Plot busy and busy violation link counts vs RU number vs trigger ID
  //----------------------------------------------------------------------------
  TH2D* h9 = new TH2D(Form("h_busy_link_count_map_layer_%i", mLayer),
                      Form("Busy Link Count - Layer %i",
                           mLayer),
                      mNumTriggers,0,mNumTriggers-1,
                      mNumReadoutUnits, -0.5, mNumReadoutUnits-0.5);
  TH2D* h10 = new TH2D(Form("h_busyv_link_count_map_layer_%i", mLayer),
                      Form("Busy Violation Link Count - Layer %i",
                           mLayer),
                      mNumTriggers,0,mNumTriggers-1,
                      mNumReadoutUnits, -0.5, mNumReadoutUnits-0.5);
  TH2D* h11 = new TH2D(Form("h_flush_link_count_map_layer_%i", mLayer),
                       Form("Flushed Incomplete Link Count - Layer %i",
                            mLayer),
                       mNumTriggers,0,mNumTriggers-1,
                       mNumReadoutUnits, -0.5, mNumReadoutUnits-0.5);
  TH2D* h12 = new TH2D(Form("h_abort_link_count_map_layer_%i", mLayer),
                       Form("Readout Abort Link Count - Layer %i",
                            mLayer),
                       mNumTriggers,0,mNumTriggers-1,
                       mNumReadoutUnits, -0.5, mNumReadoutUnits-0.5);
  TH2D* h13 = new TH2D(Form("h_fatal_link_count_map_layer_%i", mLayer),
                       Form("Fatal Mode Link Count - Layer %i",
                            mLayer),
                       mNumTriggers,0,mNumTriggers-1,
                       mNumReadoutUnits, -0.5, mNumReadoutUnits-0.5);

  // Resize and initialize vectors with counts of busy and busyv
  mBusyLinkCount.clear();
  mBusyVLinkCount.clear();
  mFlushLinkCount.clear();
  mAbortLinkCount.clear();
  mFatalLinkCount.clear();
  mBusyLinkCount.resize(mNumTriggers, 0);
  mBusyVLinkCount.resize(mNumTriggers, 0);
  mFlushLinkCount.resize(mNumTriggers, 0);
  mAbortLinkCount.resize(mNumTriggers, 0);
  mFatalLinkCount.resize(mNumTriggers, 0);

  for(unsigned int RU_num = 0; RU_num < mNumReadoutUnits; RU_num++) {
    std::vector<unsigned int> RU_busy_link_count = mRUStats[RU_num].getBusyLinkCount();
    std::vector<unsigned int> RU_busyv_link_count = mRUStats[RU_num].getBusyVLinkCount();
    std::vector<unsigned int> RU_flush_link_count = mRUStats[RU_num].getFlushLinkCount();
    std::vector<unsigned int> RU_abort_link_count = mRUStats[RU_num].getAbortLinkCount();
    std::vector<unsigned int> RU_fatal_link_count = mRUStats[RU_num].getFatalLinkCount();

    if(RU_busy_link_count.size() != mNumTriggers || RU_busyv_link_count.size() != mNumTriggers) {
      std::cout << "Error: Number of triggers in busy/busyv link count vectors from RU " << RU_num;
      std::cout << " does not matched expected number of triggers. " << std::endl;
      exit(-1);
    }

    for(uint64_t trigger_id = 0; trigger_id < mNumTriggers; trigger_id++) {
      h9->Fill(trigger_id, RU_num, RU_busy_link_count[trigger_id]);
      h10->Fill(trigger_id, RU_num, RU_busyv_link_count[trigger_id]);
      h11->Fill(trigger_id, RU_num, RU_flush_link_count[trigger_id]);
      h12->Fill(trigger_id, RU_num, RU_abort_link_count[trigger_id]);
      h13->Fill(trigger_id, RU_num, RU_fatal_link_count[trigger_id]);

      mBusyLinkCount[trigger_id] += RU_busy_link_count[trigger_id];
      mBusyVLinkCount[trigger_id] += RU_busyv_link_count[trigger_id];
      mFlushLinkCount[trigger_id] += RU_flush_link_count[trigger_id];
      mAbortLinkCount[trigger_id] += RU_abort_link_count[trigger_id];
      mFatalLinkCount[trigger_id] += RU_fatal_link_count[trigger_id];

      mNumBusyEvents += RU_busy_link_count[trigger_id];
      mNumBusyVEvents += RU_busyv_link_count[trigger_id];
      mNumFlushEvents += RU_flush_link_count[trigger_id];
      mNumAbortEvents += RU_abort_link_count[trigger_id];
      mNumFatalEvents += RU_fatal_link_count[trigger_id];
    }
  }

  h9->GetYaxis()->SetTitle("RU Number");
  h10->GetYaxis()->SetTitle("RU Number");
  h11->GetYaxis()->SetTitle("RU Number");
  h12->GetYaxis()->SetTitle("RU Number");
  h13->GetYaxis()->SetTitle("RU Number");
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

  h9->GetYaxis()->SetNdivisions(mNumReadoutUnits);
  h10->GetYaxis()->SetNdivisions(mNumReadoutUnits);
  h11->GetYaxis()->SetNdivisions(mNumReadoutUnits);
  h12->GetYaxis()->SetNdivisions(mNumReadoutUnits);
  h13->GetYaxis()->SetNdivisions(mNumReadoutUnits);

  if(create_png) {
    h9->Draw("COLZ");
    c1->Print(Form("%s/png/Layer_%i_busy_link_count_map.png",
                   mSimDataPath.c_str(), mLayer));

    h10->Draw("COLZ");
    c1->Print(Form("%s/png/Layer_%i_busyv_link_count_map.png",
                   mSimDataPath.c_str(), mLayer));

    h11->Draw("COLZ");
    c1->Print(Form("%s/png/Layer_%i_flush_link_count_map.png",
                   mSimDataPath.c_str(), mLayer));

    h12->Draw("COLZ");
    c1->Print(Form("%s/png/Layer_%i_abort_link_count_map.png",
                   mSimDataPath.c_str(), mLayer));

    h13->Draw("COLZ");
    c1->Print(Form("%s/png/Layer_%i_fatal_link_count_map.png",
                   mSimDataPath.c_str(), mLayer));
  }

  if(create_pdf) {
    h9->Draw("COLZ");
    c1->Print(Form("%s/pdf/Layer_%i_busy_link_count_map.pdf",
                   mSimDataPath.c_str(), mLayer));

    h10->Draw("COLZ");
    c1->Print(Form("%s/pdf/Layer_%i_busyv_link_count_map.pdf",
                   mSimDataPath.c_str(), mLayer));

    h11->Draw("COLZ");
    c1->Print(Form("%s/pdf/Layer_%i_flush_link_count_map.pdf",
                   mSimDataPath.c_str(), mLayer));

    h12->Draw("COLZ");
    c1->Print(Form("%s/pdf/Layer_%i_abort_link_count_map.pdf",
                   mSimDataPath.c_str(), mLayer));

    h13->Draw("COLZ");
    c1->Print(Form("%s/pdf/Layer_%i_fatal_link_count_map.pdf",
                   mSimDataPath.c_str(), mLayer));
  }

  h9->Write();
  h10->Write();
  h11->Write();
  h12->Write();
  h13->Write();

  TArrayD avg_trig_efficiency(1);
  TArrayD avg_readout_efficiency(1);
  TArrayI num_busy_events(1);
  TArrayI num_busyv_events(1);
  TArrayI num_flush_events(1);
  TArrayI num_abort_events(1);
  TArrayI num_fatal_events(1);

  avg_trig_efficiency[0] = getAvgTrigDistrEfficiency();
  avg_readout_efficiency[0] = getAvgTrigReadoutEfficiency();
  num_busy_events[0] = getNumBusyEvents();
  num_busyv_events[0] = getNumBusyVEvents();
  num_flush_events[0] = getNumFlushEvents();
  num_abort_events[0] = getNumAbortEvents();
  num_fatal_events[0] = getNumFatalEvents();

  current_dir->WriteObject(&avg_trig_efficiency, "avg_trig_efficiency");
  current_dir->WriteObject(&avg_readout_efficiency, "avg_readout_efficiency");
  current_dir->WriteObject(&num_busy_events, "num_busy_events");
  current_dir->WriteObject(&num_busyv_events, "num_busyv_events");
  current_dir->WriteObject(&num_flush_events, "num_flush_events");
  current_dir->WriteObject(&num_abort_events, "num_abort_events");
  current_dir->WriteObject(&num_fatal_events, "num_fatal_events");

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
  delete c1;

  // Go back to top level directory
  current_dir->cd();
}
