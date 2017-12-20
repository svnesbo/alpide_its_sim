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
#include "TCanvas.h"
#include "TProfile.h"
#include "TH2F.h"
#include "TH1F.h"
#include "TStyle.h"
#include "TCanvas.h"
#include "TFile.h"
#include "TDirectory.h"


ReadoutUnitStats::ReadoutUnitStats(unsigned int layer, unsigned int stave, const char* path)
  : mLayer(layer)
  , mStave(stave)
  , mSimDataPath(path)
{
  std::stringstream ss_file_path_base;
  ss_file_path_base << path << "/" << "RU_" << layer << "_" << stave;

  readTrigActionsFile(ss_file_path_base.str());
  readBusyEventFiles(ss_file_path_base.str());
  readProtocolUtilizationFile(ss_file_path_base.str());
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


///@brief Reads the RUs busy event files, and initializes LinkStats objects for each link
///       found with the various busy event data.
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
    mLinkStats.emplace_back(mLayer, mStave, link_count);

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

      // Keep track of busy time for all links, as well as for individual links (below)
      mAllBusyTime.push_back(busy_time.mBusyTimeNs);

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

      // Keep track of busy trigger lengths per link, and for all links
      mLinkStats.back().mBusyTriggerLengths.push_back(1 + (busy_off_trigger-busy_on_trigger));
      mAllBusyTriggerLengths.push_back(1 + (busy_off_trigger-busy_on_trigger));

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

    uint64_t busyv_sequence_count = 0;

    // Iterate through busy violation events for this link
    for(uint64_t event_count = 0; event_count < num_busyv_events; event_count++) {
      uint64_t busyv_trigger_id = 0;

      busyv_file.read((char*)&busyv_trigger_id, sizeof(uint64_t));

      // If this is not the first busy violation event, calculate how
      // many triggers since the previous busy violation, and calculate
      // lengths of busy violation sequences
      if(event_count > 0) {
        uint64_t prev_busyv_trigger = mLinkStats.back().mBusyVTriggers.back();
        uint64_t busyv_distance = busyv_trigger_id - prev_busyv_trigger;

        // Keep track of busyv distances per link, and for all links
        mLinkStats.back().mBusyVTriggerDistances.push_back(busyv_distance);
        mAllBusyVTriggerDistances.push_back(busyv_distance);

        if(busyv_sequence_count > 0 && busyv_distance > 1) {
          // Keep track of busyv sequences per link, and for all links
          mLinkStats.back().mBusyVTriggerSequences.push_back(busyv_sequence_count);
          mAllBusyVTriggerSequences.push_back(busyv_sequence_count);

          busyv_sequence_count = 0;
        }
      }

      busyv_sequence_count++;

      mLinkStats.back().mBusyVTriggers.push_back(busyv_trigger_id);

      std::cout << "Busy violation " << event_count << std::endl;
      std::cout << "\tTrigger id: " << busyv_trigger_id << std::endl ;
    }

    if(busyv_sequence_count > 0) {
      // Keep track of busyv sequences per link, and for all links
      mLinkStats.back().mBusyVTriggerSequences.push_back(busyv_sequence_count);
      mAllBusyVTriggerSequences.push_back(busyv_sequence_count);
    }
  }
}


///@brief Read and parse CSV file with protocol utilization stats.
///       Must be called after readBusyEventFiles(), because it needs to know
///       how many links to expect.
void ReadoutUnitStats::readProtocolUtilizationFile(std::string file_path_base)
{
  if(mLinkStats.empty()) {
    std::cout << "ReadoutUnitStats::readProtocolUtilizationFile(): called without";
    std::cout << " initializing LinkStats objects first." << std::endl;
    exit(-1);
  }

  unsigned int num_data_links = mLinkStats.size();


  std::stringstream ss_prot_util;

  ss_prot_util  << file_path_base << "_Link_utilization.csv";

  std::string prot_util_filename = ss_prot_util.str();

  std::cout << "Opening file: " << prot_util_filename << std::endl;
  std::ifstream prot_util_file(prot_util_filename, std::ios_base::in);

  if(!prot_util_file.is_open()) {
    std::cerr << "Error opening file " << prot_util_filename << std::endl;
    exit(-1);
  }

  std::string csv_header;
  std::getline(prot_util_file, csv_header);

  if(csv_header.length() == 0) {
    std::cout << "ReadoutUnitStats::readProtocolUtilizationFile(): ";
    std::cout << "Error reading or empty CSV header read." << std::endl;
    exit(-1);
  }

  unsigned int index = 0;
  while(csv_header.length() > 0) {
    size_t end_of_field_pos = csv_header.find(";");
    std::string header_field = csv_header.substr(0, end_of_field_pos);
    mProtocolUtilization[header_field] = 0;
    mProtUtilIndex[index] = header_field;

    std::cout << "Found field: " << header_field << std::endl;

    // Remove the current field, accounting for both with
    // or without semicolon at the end
    if(end_of_field_pos == std::string::npos)
      csv_header = "";
    else
      csv_header = csv_header.substr(csv_header.find(";")+1);

    index++;
  }


  for(unsigned int link_count = 0; link_count < num_data_links; link_count++) {
    if(prot_util_file.good() == false) {
      std::cout << "ReadoutUnitStats::readProtocolUtilizationFile(): CSV file not ";
      std::cout << "good before " << num_data_links << "links have been read." << std::endl;
    }

    mLinkStats[link_count].mProtUtilIndex = mProtUtilIndex;

    unsigned int index = 0;
    std::string csv_line;
    std::getline(prot_util_file, csv_line);

    while(csv_line.length() > 0) {
      size_t end_of_field_pos = csv_line.find(";");
      std::string value_str = csv_line.substr(0, end_of_field_pos);
      std::string field = mProtUtilIndex[index];

      // Update stats for both links, and combined stats for all links for in RU
      mLinkStats[link_count].mProtocolUtilization[field] = std::stoul(value_str);;
      mProtocolUtilization[field] += std::stoul(value_str);;

      // Remove the current field, accounting for both with
      // or without semicolon at the end
      if(end_of_field_pos == std::string::npos)
        csv_line = "";
      else
        csv_line = csv_line.substr(csv_line.find(";")+1);

      index++;

      if(index > 30)
        exit(-1);
    }

    if(index != mProtUtilIndex.size()) {
      std::cout << "Incorrect number of fields on line " << link_count+1;
      std::cout << " in file " << prot_util_filename << std::endl;
      exit(-1);
    }
  }


  std::cout << std::endl << std::endl;
  std::cout << "Printing link utilization stats - totals:" << std::endl;
  std::cout << "-----------------------------------------" << std::endl;

  for(auto it = mProtUtilIndex.begin(); it != mProtUtilIndex.end(); it++) {
    std::cout << it->second << ": " << mProtocolUtilization[it->second] << std::endl;
  }

  std::cout << std::endl << std::endl;

  for(unsigned int link_count = 0; link_count < num_data_links; link_count++) {
    std::cout << std::endl << std::endl;
    std::cout << "Printing link utilization stats - link " << link_count << ":" << std::endl;
    std::cout << "-----------------------------------------" << std::endl;

    for(auto it = mProtUtilIndex.begin(); it != mProtUtilIndex.end(); it++) {
      std::cout << it->second << ": " << mLinkStats[link_count].mProtocolUtilization[it->second] << std::endl;
    }
    std::cout << std::endl << std::endl;
  }
}


double ReadoutUnitStats::getTriggerCoverage(uint64_t trigger_id) const
{
  return mTriggerCoverage[trigger_id];
}


void ReadoutUnitStats::plotRU(bool create_png, bool create_pdf)
{
  unsigned int num_data_links = mLinkStats.size();

  TDirectory* current_dir = gDirectory;

  if(gDirectory == nullptr) {
    std::cout << "ReadoutUnitsStats::plotRU() error: gDirectory not initialized." << std::endl;
    exit(-1);
  }

  gDirectory->mkdir(Form("RU_%i", mStave));
  gDirectory->cd(Form("RU_%i", mStave));

  TCanvas* c1 = new TCanvas();
  c1->cd();

  //----------------------------------------------------------------------------
  // Plot busy map vs trigger id
  // Todo: Use sparse histogram?
  //----------------------------------------------------------------------------
  TH2D *h1 = new TH2D("h_busy_map","Busy events",
                              mNumTriggers,0,mNumTriggers-1,
                              num_data_links*5,0,num_data_links);

  h1->GetXaxis()->SetTitle("Trigger ID");
  h1->GetYaxis()->SetTitle("Link ID");

  std::cout << "Plotting data.. " << num_data_links << " links." << std::endl;

  for(unsigned int link_id = 0; link_id < num_data_links; link_id++) {
    for(auto busy_event_it = mLinkStats[link_id].mBusyTriggers.begin();
        busy_event_it != mLinkStats[link_id].mBusyTriggers.end();
        busy_event_it++)
    {
      h1->Fill(*busy_event_it, link_id, 1);
    }
  }

  h1->Draw("COL");
  h1->Write();

  if(create_png)
    c1->Print(Form("%s/png/RU_%i_%i_busy_map.png",
                   mSimDataPath.c_str(),
                   mLayer, mStave));
  if(create_pdf)
    c1->Print(Form("%s/pdf/RU_%i_%i_busy_map.pdf",
                   mSimDataPath.c_str(),
                   mLayer, mStave));


  //----------------------------------------------------------------------------
  // Plot busy link count vs trigger id
  //----------------------------------------------------------------------------
  TH1D *h2 = h1->ProjectionX("h_busy_links");
  h2->GetYaxis()->SetTitle("Busy link count");
  h2->GetXaxis()->SetTitle("Trigger ID");
  h2->Write();
  h2->Draw();

  if(create_png)
    c1->Print(Form("%s/png/RU_%i_%i_busy_links.png",
                   mSimDataPath.c_str(),
                   mLayer, mStave));
  if(create_pdf)
    c1->Print(Form("%s/pdf/RU_%i_%i_busy_links.pdf",
                   mSimDataPath.c_str(),
                   mLayer, mStave));


  //----------------------------------------------------------------------------
  // Plot busy violation map vs trigger id
  //----------------------------------------------------------------------------
  TH2D *h3 = new TH2D("h_busyv_map","Busy violation events",
                      mNumTriggers,0,mNumTriggers-1,
                      num_data_links,0,num_data_links);

  h3->GetXaxis()->SetTitle("Trigger ID");
  h3->GetYaxis()->SetTitle("Link ID");

  for(unsigned int link_id = 0; link_id < num_data_links; link_id++) {
    for(auto busyv_event_it = mLinkStats[link_id].mBusyVTriggers.begin();
        busyv_event_it != mLinkStats[link_id].mBusyVTriggers.end();
        busyv_event_it++)
    {
      h3->Fill(*busyv_event_it, link_id, 1);
    }
  }

  h3->Write();
  h3->Draw();

  if(create_png)
    c1->Print(Form("%s/png/RU_%i_%i_busyv_map.png",
                   mSimDataPath.c_str(),
                   mLayer, mStave));
  if(create_pdf)
    c1->Print(Form("%s/pdf/RU_%i_%i_busyv_map.pdf",
                   mSimDataPath.c_str(),
                   mLayer, mStave));


  //----------------------------------------------------------------------------
  // Plot busy link count vs trigger id
  //----------------------------------------------------------------------------
  TH1D *h4 = h3->ProjectionX("h_busyv_links");
  h4->GetYaxis()->SetTitle("Busy violation link count");
  h4->GetXaxis()->SetTitle("Trigger ID");
  h4->Write();
  h4->Draw();

  if(create_png)
    c1->Print(Form("%s/png/RU_%i_%i_busy_count_vs_trigger.png",
                   mSimDataPath.c_str(),
                   mLayer, mStave));
  if(create_pdf)
    c1->Print(Form("%s/pdf/RU_%i_%i_busy_count_vs_trigger.pdf",
                   mSimDataPath.c_str(),
                   mLayer, mStave));


  //----------------------------------------------------------------------------
  // Plot busy time distribution
  //----------------------------------------------------------------------------
  TH1D *h5 = new TH1D("h_busy_time",
                      Form("Busy time RU %i:%i", mLayer, mStave),
                      50,0,100000);
  h5->GetXaxis()->SetTitle("Time [ns]");
  h5->GetYaxis()->SetTitle("Counts");

  for(auto busy_time_it = mAllBusyTime.begin();
      busy_time_it != mAllBusyTime.end();
      busy_time_it++)
  {
    h5->Fill(*busy_time_it);
  }

  h5->SetStats(true);
  h5->Write();
  h5->Draw();

  if(create_png)
    c1->Print(Form("%s/png/RU_%i_%i_busy_time.png",
                   mSimDataPath.c_str(),
                   mLayer, mStave));
  if(create_pdf)
    c1->Print(Form("%s/pdf/RU_%i_%i_busy_time.pdf",
                   mSimDataPath.c_str(),
                   mLayer, mStave));


  //----------------------------------------------------------------------------
  // Plot busy trigger length distribution
  //----------------------------------------------------------------------------
  TH1D *h6 = new TH1D("h_busy_trigger",
                      Form("Busy trigger length RU %i:%i", mLayer, mStave),
                      64,0,64);

  h6->GetXaxis()->SetTitle("Number of triggers");
  h6->GetYaxis()->SetTitle("Counts");

  for(auto busy_trigger_it = mAllBusyTriggerLengths.begin();
      busy_trigger_it != mAllBusyTriggerLengths.end();
      busy_trigger_it++)
  {
    h6->Fill(*busy_trigger_it);
  }

  h6->SetStats(true);
  h6->Write();
  h6->Draw();

  if(create_png)
    c1->Print(Form("%s/png/RU_%i_%i_busy_trig_len.png",
                   mSimDataPath.c_str(),
                   mLayer, mStave));
  if(create_pdf)
    c1->Print(Form("%s/pdf/RU_%i_%i_busy_trig_len.pdf",
                   mSimDataPath.c_str(),
                   mLayer, mStave));


  //----------------------------------------------------------------------------
  // Plot busy violation trigger distance distribution
  //----------------------------------------------------------------------------
  TH1D *h7 = new TH1D("h_busyv_distance",
                      Form("Busy violation distances RU %i:%i", mLayer, mStave),
                      50,0,50);

  h7->GetXaxis()->SetTitle("Busy violation trigger distance");
  h7->GetYaxis()->SetTitle("Counts");

  for(auto busyv_dist_it = mAllBusyVTriggerDistances.begin();
      busyv_dist_it != mAllBusyVTriggerDistances.end();
      busyv_dist_it++)
  {
    h7->Fill(*busyv_dist_it);
  }

  h7->SetStats(true);
  h7->Write();
  h7->Draw();

  if(create_png)
    c1->Print(Form("%s/png/RU_%i_%i_busyv_distance.png",
                   mSimDataPath.c_str(),
                   mLayer, mStave));
  if(create_pdf)
    c1->Print(Form("%s/pdf/RU_%i_%i_busyv_distance.pdf",
                   mSimDataPath.c_str(),
                   mLayer, mStave));


  //----------------------------------------------------------------------------
  // Plot busy violation trigger sequence distribution
  //----------------------------------------------------------------------------
  TH1D *h8 = new TH1D("h_busyv_sequence",
                      Form("Busy violation sequences RU %i:%i", mLayer, mStave),
                      50,0,50);

  h8->GetXaxis()->SetTitle("Busy violation trigger sequence length");
  h8->GetYaxis()->SetTitle("Counts");

  for(auto busyv_dist_it = mAllBusyVTriggerSequences.begin();
      busyv_dist_it != mAllBusyVTriggerSequences.end();
      busyv_dist_it++)
  {
    h8->Fill(*busyv_dist_it);
  }

  h8->SetStats(true);
  h8->Write();
  h8->Draw();

  if(create_png)
    c1->Print(Form("%s/png/RU_%i_%i_busyv_sequence.png",
                   mSimDataPath.c_str(),
                   mLayer, mStave));
  if(create_pdf)
    c1->Print(Form("%s/pdf/RU_%i_%i_busyv_sequence.pdf",
                   mSimDataPath.c_str(),
                   mLayer, mStave));


  //----------------------------------------------------------------------------
  // Plot link utilization histogram
  //----------------------------------------------------------------------------
  TH1D *h9 = new TH1D("h_prot_util",
                      Form("Protocol utilization RU %i:%i", mLayer, mStave),
                      mProtocolUtilization.size(),
                      0,
                      mProtocolUtilization.size()-1);

  //h9->GetXaxis()->SetTitle("Data word type");
  h9->GetYaxis()->SetTitle("Counts");

  for(auto prot_util_it = mProtUtilIndex.begin();
      prot_util_it != mProtUtilIndex.end();
      prot_util_it++)
  {
    unsigned int bin_index = prot_util_it->first + 1;
    std::string bin_name = prot_util_it->second;
    h9->Fill(bin_index, mProtocolUtilization[bin_name]);
    h9->GetXaxis()->SetBinLabel(bin_index, bin_name.c_str());
  }

  // Draw labels on X axis vertically
  h9->LabelsOption("v", "x");

  //h9->SetStats(true);
  h9->Write();
  h9->Draw();

  if(create_png)
    c1->Print(Form("%s/png/RU_%i_%i_prot_utilization.png",
                   mSimDataPath.c_str(),
                   mLayer, mStave));
  if(create_pdf)
    c1->Print(Form("%s/pdf/RU_%i_%i_prot_utilization.pdf",
                   mSimDataPath.c_str(),
                   mLayer, mStave));



  //----------------------------------------------------------------------------
  // Plot link histograms
  //----------------------------------------------------------------------------
  for(unsigned int link_id = 0; link_id < num_data_links; link_id++) {
    // Keep changing back to this RU's directory,
    // because the plotLink() function changes the current directory.
    current_dir->cd(Form("RU_%i", mStave));

    mLinkStats[link_id].plotLink();
  }


  delete c1;
  delete h1;
  delete h2;
  delete h3;
  delete h4;
  delete h5;
  delete h6;
  delete h7;
  delete h8;
  delete h9;
}
