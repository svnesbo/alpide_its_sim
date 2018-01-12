/**
 * @file   ITSLayerStats.cpp
 * @author Simon Voigt Nesbo
 * @date   November 20, 2017
 * @brief  Statistics for one layer in ITS detector
 *
 */

#include "ITSLayerStats.hpp"
#include <iostream>
#include "TFile.h"
#include "TDirectory.h"
#include "TCanvas.h"
#include "TH2F.h"
#include "TH1F.h"
#include "THStack.h"

ITSLayerStats::ITSLayerStats(unsigned int layer_num, unsigned int num_staves,
                             const char* path,
                             bool create_png, bool create_pdf)
  : mLayer(layer_num)
{
  TDirectory* current_dir = gDirectory;

  if(gDirectory == nullptr) {
    std::cout << "ITSLayerStats::ITSLayerStats() error: gDirectory not initialized." << std::endl;
    exit(-1);
  }

  gDirectory->mkdir(Form("Layer_%i", mLayer));

  // Create and parse RU data, and generate plots in TFile
  for(unsigned int stave = 0; stave < num_staves; stave++) {
    // Keep changing back to this layer's directory,
    // because the plotRU() function changes the current directory.
    current_dir->cd(Form("Layer_%i", mLayer));

    mRUStats.emplace_back(layer_num, stave, path);

    mRUStats.back().plotRU(create_png, create_pdf);
  }

  current_dir->cd(Form("Layer_%i", mLayer));


  TCanvas* c1 = new TCanvas();
  c1->cd();


  // Todo: check that we have the same number of triggers in all RUs?
  uint64_t num_triggers = mRUStats[0].getNumTriggers();

  mTrigSentCoverage.resize(num_triggers);
  mTrigSentExclFilteringCoverage.resize(num_triggers);
  mTrigReadoutCoverage.resize(num_triggers);
  mTrigReadoutExclFilteringCoverage.resize(num_triggers);


  //----------------------------------------------------------------------------
  // Plot average trigger distribution and readout coverage vs. trigger
  //----------------------------------------------------------------------------
  TH1D *h1 = new TH1D("h_avg_trig_ctrl_link_coverage",
                      Form("Average Trigger Distribution Coverage - Layer %i", layer_num),
                      num_triggers,0,num_triggers-1);
  TH1D *h2 = new TH1D("h_avg_trig_ctrl_link_excl_filter_coverage",
                       Form("Average Trigger Distribution Coverage Excluding Filtering - Layer %i", layer_num),
                       num_triggers,0,num_triggers-1);
  TH1D *h3 = new TH1D("h_avg_trig_readout_coverage",
                       Form("Average Trigger Readout Coverage - Layer %i", layer_num),
                       num_triggers,0,num_triggers-1);
  TH1D *h4 = new TH1D("h_avg_trig_readout_excl_filter_coverage",
                       Form("Average Trigger Readout Coverage Excluding Filtering - Layer %i", layer_num),
                       num_triggers,0,num_triggers-1);

  for(uint64_t trigger_id = 0; trigger_id < num_triggers; trigger_id++) {
    double trig_sent_coverage = 0.0;
    double trig_sent_excl_filter_coverage = 0.0;
    double trig_readout_coverage = 0.0;
    double trig_readout_excl_filter_coverage = 0.0;
    for(unsigned int stave = 0; stave < num_staves; stave++) {
      trig_sent_coverage += mRUStats[stave].getTrigSentCoverage(trigger_id);
      trig_sent_excl_filter_coverage += mRUStats[stave].getTrigSentExclFilteringCoverage(trigger_id);
      trig_readout_coverage += mRUStats[stave].getTrigReadoutCoverage(trigger_id);
      trig_readout_excl_filter_coverage += mRUStats[stave].getTrigReadoutExclFilteringCoverage(trigger_id);
    }

    mTrigSentCoverage[trigger_id] = trig_sent_coverage/num_staves;
    mTrigSentExclFilteringCoverage[trigger_id] = trig_sent_excl_filter_coverage/num_staves;
    mTrigReadoutCoverage[trigger_id] = trig_sent_coverage/num_staves;
    mTrigReadoutExclFilteringCoverage[trigger_id] = trig_readout_excl_filter_coverage/num_staves;

    h1->Fill(trigger_id, mTrigSentCoverage[trigger_id]);
    h2->Fill(trigger_id, mTrigSentExclFilteringCoverage[trigger_id]);
    h3->Fill(trigger_id, mTrigReadoutCoverage[trigger_id]);
    h4->Fill(trigger_id, mTrigReadoutExclFilteringCoverage[trigger_id]);

    std::cout << "Layer " << layer_num << ", trigger sent" << trigger_id;
    std::cout << " coverage: " << mTrigSentCoverage[trigger_id] << std::endl;

    std::cout << "Layer " << layer_num << ", trigger sent" << trigger_id;
    std::cout << " coverage (excluding filtering): ";
    std::cout << mTrigSentExclFilteringCoverage[trigger_id] << std::endl;

    std::cout << "Layer " << layer_num << ", trigger readout" << trigger_id;
    std::cout << " coverage: " << mTrigReadoutCoverage[trigger_id] << std::endl;

    std::cout << "Layer " << layer_num << ", trigger readout" << trigger_id;
    std::cout << " coverage (excluding filtering): ";
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
    c1->Print(Form("%s/png/Layer_%i_avg_trig_ctrl_link_coverage.png",
                   path, layer_num));

    h2->Draw();
    c1->Print(Form("%s/png/Layer_%i_avg_trig_ctrl_link_excl_filter_coverage.png",
                   path, layer_num));

    h3->Draw();
    c1->Print(Form("%s/png/Layer_%i_avg_trig_readout_coverage.png",
                   path, layer_num));

    h4->Draw();
    c1->Print(Form("%s/png/Layer_%i_avg_trig_readout_excl_filter_coverage.png",
                   path, layer_num));
  }

  if(create_pdf) {
    h1->Draw();
    c1->Print(Form("%s/pdf/Layer_%i_avg_trig_ctrl_link_coverage.pdf",
                   path, layer_num));

    h2->Draw();
    c1->Print(Form("%s/pdf/Layer_%i_avg_trig_ctrl_link_excl_filter_coverage.pdf",
                   path, layer_num));

    h3->Draw();
    c1->Print(Form("%s/pdf/Layer_%i_avg_trig_readout_coverage.pdf",
                   path, layer_num));

    h4->Draw();
    c1->Print(Form("%s/pdf/Layer_%i_avg_trig_readout_excl_filter_coverage.pdf",
                   path, layer_num));
  }

  h1->Write();
  h2->Write();
  h3->Write();
  h4->Write();


  //----------------------------------------------------------------------------
  // Plot trigger distribution and readout coverage vs. RU vs. trigger
  //----------------------------------------------------------------------------
  TH2D* h5 = new TH2D(Form("h_trig_ctrl_link_coverage_layer_%i", layer_num),
                      Form("Trigger Distribution Coverage - Layer %i",
                           layer_num),
                      num_triggers,0,num_triggers-1,
                      num_staves, -0.5, num_staves-0.5);
  TH2D* h6 = new TH2D(Form("h_trig_ctrl_link_excl_filter_coverage_layer_%i", layer_num),
                      Form("Trigger Distribution Coverage Excluding Filtering - Layer %i",
                           layer_num),
                      num_triggers,0,num_triggers-1,
                      num_staves, -0.5, num_staves-0.5);
  TH2D* h7 = new TH2D(Form("h_trig_readout_coverage_layer_%i", layer_num),
                      Form("Trigger Readout Coverage - Layer %i", layer_num),
                      num_triggers,0,num_triggers-1,
                      num_staves, -0.5, num_staves-0.5);
  TH2D* h8 = new TH2D(Form("h_trig_readout_excl_filter_coverage_layer_%i", layer_num),
                      Form("Trigger Readout Coverage Excluding Filtering - Layer %i",
                           layer_num),
                      num_triggers,0,num_triggers-1,
                      num_staves, -0.5, num_staves-0.5);

  for(unsigned int stave = 0; stave < num_staves; stave++) {
    for(uint64_t trigger_id = 0; trigger_id < num_triggers; trigger_id++) {
      h5->Fill(trigger_id, stave, mRUStats[stave].getTrigSentCoverage(trigger_id));
      h6->Fill(trigger_id, stave, mRUStats[stave].getTrigSentExclFilteringCoverage(trigger_id));
      h7->Fill(trigger_id, stave, mRUStats[stave].getTrigReadoutCoverage(trigger_id));
      h8->Fill(trigger_id, stave, mRUStats[stave].getTrigReadoutExclFilteringCoverage(trigger_id));
    }
  }

  h5->GetYaxis()->SetTitle("Stave/RU Number");
  h6->GetYaxis()->SetTitle("Stave/RU Number");
  h7->GetYaxis()->SetTitle("Stave/RU Number");
  h8->GetYaxis()->SetTitle("Stave/RU Number");
  h5->GetXaxis()->SetTitle("Trigger ID");
  h6->GetXaxis()->SetTitle("Trigger ID");
  h7->GetXaxis()->SetTitle("Trigger ID");
  h8->GetXaxis()->SetTitle("Trigger ID");

  h5->GetYaxis()->SetNdivisions(num_staves);
  h6->GetYaxis()->SetNdivisions(num_staves);
  h7->GetYaxis()->SetNdivisions(num_staves);
  h8->GetYaxis()->SetNdivisions(num_staves);


  if(create_png) {
    h5->Draw("COLZ");
    c1->Print(Form("%s/png/Layer_%i_trig_ctrl_link_coverage.png",
                   path, layer_num));

    h6->Draw("COLZ");
    c1->Print(Form("%s/png/Layer_%i_trig_ctrl_link_excl_filter_coverage.png",
                   path, layer_num));

    h7->Draw("COLZ");
    c1->Print(Form("%s/png/Layer_%i_trig_readout_coverage.png",
                   path, layer_num));

    h8->Draw("COLZ");
    c1->Print(Form("%s/png/Layer_%i_trig_readout_excl_filter_coverage.png",
                   path, layer_num));
  }

  if(create_pdf) {
    h5->Draw("COLZ");
    c1->Print(Form("%s/pdf/Layer_%i_trig_ctrl_link_coverage.pdf",
                   path, layer_num));

    h6->Draw("COLZ");
    c1->Print(Form("%s/pdf/Layer_%i_trig_ctrl_link_excl_filter_coverage.pdf",
                   path, layer_num));

    h7->Draw("COLZ");
    c1->Print(Form("%s/pdf/Layer_%i_trig_readout_coverage.pdf",
                   path, layer_num));

    h8->Draw("COLZ");
    c1->Print(Form("%s/pdf/Layer_%i_trig_readout_excl_filter_coverage.pdf",
                   path, layer_num));
  }

  h5->Write();
  h6->Write();
  h7->Write();
  h8->Write();



  //----------------------------------------------------------------------------
  // Plot busy and busy violation link counts vs RU number vs trigger ID
  //----------------------------------------------------------------------------
  TH2D* h9 = new TH2D(Form("h_busy_link_count_map_layer_%i", layer_num),
                      Form("Busy Link Count - Layer %i",
                           layer_num),
                      num_triggers,0,num_triggers-1,
                      num_staves, -0.5, num_staves-0.5);
  TH2D* h10 = new TH2D(Form("h_busyv_link_count_map_layer_%i", layer_num),
                      Form("Busy Violation Link Count - Layer %i",
                           layer_num),
                      num_triggers,0,num_triggers-1,
                      num_staves, -0.5, num_staves-0.5);

  for(unsigned int stave = 0; stave < num_staves; stave++) {
    std::vector<unsigned int> RU_busy_link_count = mRUStats[stave].getBusyLinkCount();
    std::vector<unsigned int> RU_busyv_link_count = mRUStats[stave].getBusyVLinkCount();

    if(RU_busy_link_count.size() != num_triggers || RU_busyv_link_count.size() != num_triggers) {
      std::cout << "Error: Number of triggers in busy/busyv link count vectors from RU " << stave;
      std::cout << " does not matched expected number of triggers. " << std::endl;
      exit(-1);
    }

    for(uint64_t trigger_id = 0; trigger_id < num_triggers; trigger_id++) {
      h9->Fill(trigger_id, stave, RU_busy_link_count[trigger_id]);
      h10->Fill(trigger_id, stave, RU_busyv_link_count[trigger_id]);
    }
  }

  h9->GetYaxis()->SetTitle("Stave/RU Number");
  h10->GetYaxis()->SetTitle("Stave/RU Number");
  h9->GetXaxis()->SetTitle("Trigger ID");
  h10->GetXaxis()->SetTitle("Trigger ID");

  h9->GetYaxis()->SetNdivisions(num_staves);
  h10->GetYaxis()->SetNdivisions(num_staves);

  if(create_png) {
    h9->Draw("COLZ");
    c1->Print(Form("%s/png/Layer_%i_busy_link_count_map.png",
                   path, layer_num));

    h10->Draw("COLZ");
    c1->Print(Form("%s/png/Layer_%i_busyv_link_count_map.png",
                   path, layer_num));
  }

  if(create_pdf) {
    h9->Draw("COLZ");
    c1->Print(Form("%s/pdf/Layer_%i_busy_link_count_map.pdf",
                   path, layer_num));

    h10->Draw("COLZ");
    c1->Print(Form("%s/pdf/Layer_%i_busyv_link_count_map.pdf",
                   path, layer_num));
  }

  h9->Write();
  h10->Write();


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
}
