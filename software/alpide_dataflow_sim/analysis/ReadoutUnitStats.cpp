/**
 * @file   ReadoutUnitStats.cpp
 * @author Simon Voigt Nesbo
 * @date   November 20, 2017
 * @brief  ReadoutUnitStats member functions for analyzing
 *         a RU_<layer>_<stave>_Trigger_stats.csv file
 *
 */


#include "ReadoutUnitStats.hpp"
#include <sstream>
#include <fstream>
#include <iostream>
#include <cstdlib>

#include "TCanvas.h"
#include "TROOT.h"
//#include "TRandom3.h"
#include "TCanvas.h"
#include "TProfile.h"
//#include "TGraphErrors.h"
#include "TH2F.h"
#include "TH1F.h"
//#include "TLegend.h"
//#include "TArrow.h"
/* #include "TLatex.h" */
#include "TStyle.h"
#include "TCanvas.h"
#include "TFile.h"
/* #include "TPad.h" */
/* #include "TRandom.h" */


ReadoutUnitStats::ReadoutUnitStats(unsigned int layer, unsigned int stave, const char* path)
  : mLayer(layer)
  , mStave(stave)
{
  std::stringstream ss_file_path_base;
  ss_file_path_base << path << "/" << "RU_" << layer << "_" << stave;

  readTrigActionsFile(ss_file_path_base.str());
  readBusyEventFiles(ss_file_path_base.str());
}


void ReadoutUnitStats::readTrigActionsFile(std::string file_path_base)
{
  std::stringstream ss_filename;
  ss_filename  << file_path_base << "_trigger_actions.dat";
  std::string filename = ss_filename.str();

  std::cout << "Opening file: " << filename << std::endl;
  std::ifstream ru_stats_file(filename.c_str(), std::ios_base::in | std::ios_base::binary);

  if(!ru_stats_file.is_open()) {
    std::cerr << "Error opening file " << filename << std::endl;
    exit(-1);
  }

  uint64_t num_triggers;
  uint8_t num_ctrl_links;

  std::cout << "sizeof(uint8_t): " << sizeof(uint8_t) << std::endl;

  ru_stats_file.read((char*)&num_triggers, sizeof(num_triggers));
  ru_stats_file.read((char*)&num_ctrl_links, sizeof(num_ctrl_links));

  mNumTriggers = num_triggers;
  mNumCtrlLinks = num_ctrl_links;

  std::cout << "Num triggers: " << num_triggers << std::endl;
  std::cout << "Num links: " << mNumCtrlLinks << std::endl;

  mTriggerCoverage.resize(num_triggers);
  mTriggerActions.resize(num_triggers);

  uint64_t trigger_id = 0;
  uint8_t trig_action = 0;
  uint64_t unknown_trig_action_count = 0;

  // Read trigger action byte, one per link, for each trigger ID,
  // and calculate coverage etc.
  while(trigger_id < num_triggers && ru_stats_file.good()) {
    uint8_t coverage = 0;
    uint8_t links_included = 0;
    uint8_t links_filtered = 0;

    for(unsigned int link_id = 0; link_id < num_ctrl_links; link_id++) {
      ru_stats_file.read((char*)&trig_action, sizeof(uint8_t));
      //mTriggerActions[trigger_id][link_id] = trig_action;
      mTriggerActions[trigger_id].push_back(trig_action);

      std::cout << "Trigger " << trigger_id;
      std::cout << ": RU" << mLayer << "." << mStave << ": ";

      switch(trig_action) {
      case TRIGGER_SENT:
        std::cout << "TRIGGER_SENT" << std::endl;
        coverage++;
        links_included++;
        break;

      case TRIGGER_NOT_SENT_BUSY:
        std::cout << "TRIGGER_NOT_SENT_BUSY" << std::endl;
        links_included++;
        break;

      case TRIGGER_FILTERED:
        std::cout << "TRIGGER_FILTERED" << std::endl;
        links_filtered++;
        break;
      default:
        std::cout << "UNKNOWN" << std::endl;
        unknown_trig_action_count++;
        break;
      }
    }

    if(links_included == 0)
      mTriggerCoverage[trigger_id] = 0.0;
    else
      mTriggerCoverage[trigger_id] = (double)coverage/links_included;

    std::cout << "Trigger " << trigger_id;
    std::cout << " coverage: " << mTriggerCoverage[trigger_id] << std::endl;

    if(links_filtered != 0 && links_filtered != num_ctrl_links)
      mTriggerMismatch.push_back(trigger_id);

    trigger_id++;
  }

  if(trigger_id != num_triggers) {
    std::cerr << "Error reading " << num_triggers << ", got only " << trigger_id << std::endl;
    exit(-1);
  }

  std::cout << "Number of unknown trigger actions: " << unknown_trig_action_count << std::endl;

  std::cout << "Links with filter mismatch: ";
  for(auto it = mTriggerMismatch.begin(); it != mTriggerMismatch.end(); it++) {
    if(it == mTriggerMismatch.begin())
       std::cout << *it;
    else
      std::cout << ", " << *it;
  }
  std::cout << std::endl;
}


///@brief
void ReadoutUnitStats::readBusyEventFiles(std::string file_path_base)
{
  std::stringstream ss_busy_events;
  std::stringstream ss_busyv_events;

  ss_busy_events  << file_path_base << "_busy_events.dat";
  ss_busyv_events << file_path_base << "_busyv_events.dat";

  std::string busy_events_filename = ss_busy_events.str();
  std::string busyv_events_filename = ss_busyv_events.str();

  std::cout << "Opening file: " << busy_events_filename << std::endl;
  std::ifstream busy_file(busy_events_filename, std::ios_base::in | std::ios_base::binary);

  if(!busy_file.is_open()) {
    std::cerr << "Error opening file " << busy_events_filename << std::endl;
    exit(-1);
  }

  std::cout << "Opening file: " << busyv_events_filename << std::endl;
  std::ifstream busyv_file(busyv_events_filename, std::ios_base::in | std::ios_base::binary);

  if(!busyv_file.is_open()) {
    std::cerr << "Error opening file " << busyv_events_filename << std::endl;
    exit(-1);
  }

  uint8_t num_data_links_busy_file = 0;
  uint8_t num_data_links_busyv_file = 0;

  busy_file.read((char*)&num_data_links_busy_file, sizeof(uint8_t));
  busyv_file.read((char*)&num_data_links_busyv_file, sizeof(uint8_t));

  if(num_data_links_busy_file != num_data_links_busyv_file) {
    std::cerr << "Error: " << num_data_links_busy_file << " data links in busy file,";
    std::cerr << " does not equal " << num_data_links_busyv_file << " data links in busyv file";
    std::cerr << std::endl;
    exit(-1);
  }

  std::cout << std::endl << std::endl;
  std::cout << "Number of data links: ";
  std::cout << int(num_data_links_busy_file);
  std::cout << std::endl;
  std::cout << "-------------------------------------------------" << std::endl;

  // Iterate through data for each link
  for(uint8_t link_count = 0; link_count < num_data_links_busy_file; link_count++) {

    std::cout << "Data link " << int(link_count) << std::endl;

    // Add a new LinkStats entry
    mLinkStats.push_back(LinkStats());

    // Number of busy events for this link
    uint64_t num_busy_events = 0;
    busy_file.read((char*)&num_busy_events, sizeof(uint64_t));

    // Iterate through busy events for this link
    for(uint64_t event_count = 0; event_count < num_busy_events; event_count++) {
      BusyTime busy_time;
      uint64_t busy_on_trigger = 0;
      uint64_t busy_off_trigger = 0;

      busy_file.read((char*)&busy_time.mStartTimeNs, sizeof(uint64_t));
      busy_file.read((char*)&busy_time.mEndTimeNs, sizeof(uint64_t));

      busy_time.mBusyTimeNs = busy_time.mEndTimeNs - busy_time.mStartTimeNs;

      busy_file.read((char*)&busy_on_trigger, sizeof(uint64_t));
      busy_file.read((char*)&busy_off_trigger, sizeof(uint64_t));

      // Add entry with data about when the link went busy,
      // and when it went out of busy, for this busy event
      mLinkStats.back().mBusyTime.push_back(busy_time);

      uint64_t trigger_id = busy_on_trigger;

      // Add an entry for each trigger that was within the busy interval
      // (regardless of for "how long" the busy was for a specific trigger)
      // The preceding trigger that led to the busy event is also counted in here
      do {
        mLinkStats.back().mBusyTriggers.push_back(trigger_id);
        trigger_id++;
      } while(trigger_id < busy_off_trigger);

      mLinkStats.back().mBusyTriggerLengths.push_back(1 + (busy_off_trigger-busy_on_trigger));

      std::cout << "Busy event " << event_count << std::endl;
      std::cout << "\tBusy on time: " << busy_time.mStartTimeNs << std::endl;
      std::cout << "\tBusy off time: " << busy_time.mEndTimeNs << std::endl;
      std::cout << "\tBusy time: " << busy_time.mBusyTimeNs << std::endl;

      std::cout << "\tBusy on trigger: " << busy_on_trigger << std::endl;
      std::cout << "\tBusy off trigger: " << busy_off_trigger << std::endl;
    }


    // Number of busy violation events for this link
    uint64_t num_busyv_events = 0;
    busyv_file.read((char*)&num_busyv_events, sizeof(uint64_t));

    // Iterate through busy violation events for this link
    for(uint64_t event_count = 0; event_count < num_busyv_events; event_count++) {
      uint64_t busyv_trigger_id = 0;

      busyv_file.read((char*)&busyv_trigger_id, sizeof(uint64_t));

      // If this is not the first busy violation event, calculate how
      // many triggers since the previous busy violation, and store it
      if(event_count > 0) {
        uint64_t prev_busyv_trigger = mLinkStats.back().mBusyViolationTriggers.back();
        uint64_t busyv_distance = busyv_trigger_id - prev_busyv_trigger;
        mLinkStats.back().mBusyVTriggerDistances.push_back(busyv_distance);
      }

      mLinkStats.back().mBusyViolationTriggers.push_back(busyv_trigger_id);

      std::cout << "Busy violation " << event_count << std::endl;
      std::cout << "\tTrigger id: " << busyv_trigger_id << std::endl ;
    }
  }
}


double ReadoutUnitStats::getTriggerCoverage(uint64_t trigger_id) const
{
  return mTriggerCoverage[trigger_id];
}


void ReadoutUnitStats::plotRU(void)
{
  unsigned int num_data_links = mLinkStats.size();

  std::stringstream ss_root_filename;
  ss_root_filename << "RU_" << mLayer << "_" << mStave << ".root";

  TFile *f = new TFile(ss_root_filename.str().c_str(), "recreate");

  std::stringstream ss_busy_canvas_name;
  ss_busy_canvas_name << "c_" << mLayer << "_" << mStave << "_busy";

  TCanvas *c1 = new TCanvas(ss_busy_canvas_name.str().c_str(),
                            ss_busy_canvas_name.str().c_str(),
                            900,900);
  gStyle->SetOptStat(0);

  // Create the three pads
  /* TPad *center_pad = new TPad("center_pad", "center_pad",0.0,0.0,0.6,0.6); */
  /* center_pad->Draw(); */

  c1->cd();

  TPad *pad = new TPad("pad", "pad", 0.0,0.0,1.0,1.0);
  pad->Draw();


  /* TPad *right_pad = new TPad("right_pad", "right_pad",0.55,0.0,1.0,0.6); */
  /* right_pad->Draw(); */

  /* TPad *top_pad = new TPad("top_pad", "top_pad",0.0,0.55,0.6,1.0); */
  /* top_pad->Draw(); */



  // Create, fill and project a 2D histogram.
  TH2D *h_busy_map = new TH2D("h_busy_map","Busy events",
                      mNumTriggers,0,mNumTriggers-1,
                      num_data_links,0,num_data_links);

  /* Float_t px, py; */
  /* for (Int_t i = 0; i < 25000; i++) { */
  /*    gRandom->Rannor(px,py); */
  /*    h2->Fill(px,5*py); */
  /* } */

  std::cout << "Plotting data.. " << num_data_links << " links." << std::endl;

  for(unsigned int link_id = 0; link_id < num_data_links; link_id++) {
    for(auto busy_event_it = mLinkStats[link_id].mBusyTriggers.begin();
        busy_event_it != mLinkStats[link_id].mBusyTriggers.end();
        busy_event_it++)
    {
      h_busy_map->Fill(*busy_event_it, link_id, 1);
    }
  }


  /* for(Int_t trigger_id = 0; trigger_id < 101; trigger_id++) { */
  /*   for(Int_t link_id = 0; link_id < 11; link_id++) { */
  /*     int val = gRandom->Integer(2); // Get random 0 or 1 */

  /*     if(val == 1) */
  /*       h2->Fill(trigger_id, link_id); */
  /*   } */
  /* } */

  /* TH1D * projh2X = h2->ProjectionX(); */
  /* TH1D * projh2Y = h2->ProjectionY(); */


  pad->cd();
  // Drawing
  //center_pad->cd();
  gStyle->SetPalette(1);
  //h2->SetOption("LEGO");
  //h2->Draw("COL2");
  h_busy_map->Draw("BOX");
  h_busy_map->Write();

  c1->Update();

  /* top_pad->cd(); */
  /* projh2X->SetFillColor(kBlue+1); */
  /* projh2X->Draw("bar"); */

  /* right_pad->cd(); */
  /* projh2Y->SetFillColor(kBlue-2); */
  /* projh2Y->Draw("hbar"); */

  /* c1->cd(); */
  /* TLatex *t = new TLatex(); */
  /* t->SetTextFont(42); */
  /* t->SetTextSize(0.02); */
  /* t->DrawLatex(0.6,0.88,"This example demonstrate how to display"); */
  /* t->DrawLatex(0.6,0.85,"a histogram and its two projections."); */


  //----------------------------------------------------------------------------
  // Plot busy time distribution
  //----------------------------------------------------------------------------
  std::stringstream ss_busy_time_distr_canvas_name;
  ss_busy_time_distr_canvas_name << "c_" << mLayer << "_" << mStave << "_busy_time_distr";

  TCanvas *c2 = new TCanvas(ss_busy_time_distr_canvas_name.str().c_str(),
                            ss_busy_time_distr_canvas_name.str().c_str(),
                            900,900);

  c2->cd();

  TPad *pad_busy_time_distr = new TPad("pad_busy_time_distr",
                                       "pad_busy_time_distr",
                                       0.0,0.0,1.0,1.0);
  pad_busy_time_distr->Draw();
  pad_busy_time_distr->cd();

  for(unsigned int link_id = 0; link_id < num_data_links; link_id++) {
    std::string h_name = std::string("h_busy_time_distr_link_") + std::to_string(link_id);

    TH1D *h_busy_time_distr = new TH1D(h_name.c_str(),"Busy time",50,0,100000);
    //h_busy_time_distr->SetNameTitle(h_name.c_str(),"Busy time");

    //gStyle->SetHistLineColor(link_id);

    for(auto busy_time_it = mLinkStats[link_id].mBusyTime.begin();
        busy_time_it != mLinkStats[link_id].mBusyTime.end();
        busy_time_it++)
    {
      h_busy_time_distr->Fill(busy_time_it->mBusyTimeNs);
    }

    //h_busy_time->Draw("CONT SAME");
    h_busy_time_distr->Write();
    //link_id = num_data_links;
  }

  c2->Update();


  //----------------------------------------------------------------------------
  // Plot busy trigger length distribution
  //----------------------------------------------------------------------------
  std::stringstream ss_busy_trigger_distr_canvas_name;
  ss_busy_trigger_distr_canvas_name << "c_" << mLayer;
  ss_busy_trigger_distr_canvas_name << "_" << mStave << "_busy_trigger_distr";

  TCanvas *c3 = new TCanvas(ss_busy_trigger_distr_canvas_name.str().c_str(),
                            ss_busy_trigger_distr_canvas_name.str().c_str(),
                            900,900);

  c3->cd();

  TPad *pad_busy_trigger_distr = new TPad("pad_busy_trigger_distr",
                                          "pad_busy_trigger_distr",
                                          0.0,0.0,1.0,1.0);
  pad_busy_trigger_distr->Draw();
  pad_busy_trigger_distr->cd();

  for(unsigned int link_id = 0; link_id < num_data_links; link_id++) {
    std::string h_name = std::string("h_busy_trigger_distr_link_") +
                         std::to_string(link_id);

    TH1D *h_busy_trigger_distr = new TH1D(h_name.c_str(),"Busy trigger lengths",64,0,64);
    //h_busy_trigger_distr->SetNameTitle(h_name.c_str(),"Busy trigger");

    //gStyle->SetHistLineColor(link_id);

    for(auto busy_trigger_it = mLinkStats[link_id].mBusyTriggerLengths.begin();
        busy_trigger_it != mLinkStats[link_id].mBusyTriggerLengths.end();
        busy_trigger_it++)
    {
      h_busy_trigger_distr->Fill(*busy_trigger_it);
    }

    //h_busy_trigger->Draw("CONT SAME");
    h_busy_trigger_distr->Write();
    //link_id = num_data_links;
  }

  c3->Update();


  //----------------------------------------------------------------------------
  // Plot busy violation trigger distance distribution
  //----------------------------------------------------------------------------
  std::stringstream ss_busyv_dist_distr_canvas_name;
  ss_busyv_dist_distr_canvas_name << "c_" << mLayer;
  ss_busyv_dist_distr_canvas_name << "_" << mStave << "_busyv_distance_distr";

  TCanvas *c4 = new TCanvas(ss_busyv_dist_distr_canvas_name.str().c_str(),
                            ss_busyv_dist_distr_canvas_name.str().c_str(),
                            900,900);

  c4->cd();

  TPad *pad_busyv_dist_distr = new TPad("pad_busyv_dist_distr",
                                        "pad_busyv_dist_distr",
                                        0.0,0.0,1.0,1.0);
  pad_busyv_dist_distr->Draw();
  pad_busyv_dist_distr->cd();

  for(unsigned int link_id = 0; link_id < num_data_links; link_id++) {
    std::string h_name = std::string("h_busyv_distance_distr_link_") +
                         std::to_string(link_id);

    TH1D *h_busyv_dist_distr = new TH1D(h_name.c_str(),
                                        "Busy violation trigger distances",
                                        50,0,50);
    //h_busy_trigger_distr->SetNameTitle(h_name.c_str(),"Busy trigger");

    //gStyle->SetHistLineColor(link_id);

    for(auto busyv_dist_it = mLinkStats[link_id].mBusyVTriggerDistances.begin();
        busyv_dist_it != mLinkStats[link_id].mBusyVTriggerDistances.end();
        busyv_dist_it++)
    {
      h_busyv_dist_distr->Fill(*busyv_dist_it);
    }

    //h_busy_trigger->Draw("CONT SAME");
    h_busyv_dist_distr->Write();
    //link_id = num_data_links;
  }

  c4->Update();


  delete f;
}
