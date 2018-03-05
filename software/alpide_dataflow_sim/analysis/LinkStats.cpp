/**
 * @file   LinkStats.cpp
 * @author Simon Voigt Nesbo
 * @date   December 11, 2017
 * @brief  Statistics for one ALPIDE link
 *
 */


#include "LinkStats.hpp"
#include <sstream>
#include <fstream>
#include <iostream>
#include "TH1F.h"
#include "TFile.h"
#include "TDirectory.h"
#include "TStyle.h"


void LinkStats::plotLink(void)
{
  if(gDirectory == nullptr) {
    std::cout << "LinkStats::plotLink() error: gDirectory not initialized." << std::endl;
    exit(-1);
  }

  gDirectory->mkdir(Form("Link_%i", mLink));
  gDirectory->cd(Form("Link_%i", mLink));


  //----------------------------------------------------------------------------
  // Plot busy time distribution
  //----------------------------------------------------------------------------
  TH1D *h1 = new TH1D("h_busy_time",
                      Form("Busy time link %i:%i:%i", mLayer, mStave, mLink),
                      50,0,100000);
  h1->GetXaxis()->SetTitle("Time [ns]");
  h1->GetYaxis()->SetTitle("Counts");

  for(auto busy_time_it = mBusyTime.begin();
      busy_time_it != mBusyTime.end();
      busy_time_it++)
  {
    h1->Fill(busy_time_it->mBusyTimeNs);
  }

  gStyle->SetOptStat("men");
  h1->SetStats(true);
  h1->Write();


  //----------------------------------------------------------------------------
  // Plot busy trigger length distribution
  //----------------------------------------------------------------------------
  TH1D *h2 = new TH1D("h_busy_trigger",
                      Form("Busy trigger length link %i:%i:%i", mLayer, mStave, mLink),
                      64,0,64);

  h2->GetXaxis()->SetTitle("Number of triggers");
  h2->GetYaxis()->SetTitle("Counts");

  for(auto busy_trigger_it = mBusyTriggerLengths.begin();
      busy_trigger_it != mBusyTriggerLengths.end();
      busy_trigger_it++)
  {
    h2->Fill(*busy_trigger_it);
  }

  gStyle->SetOptStat("men");
  h2->SetStats(true);
  h2->Write();


  //----------------------------------------------------------------------------
  // Plot busy violation trigger distance distribution
  //----------------------------------------------------------------------------
  TH1D *h3 = new TH1D("h_busyv_distance",
                      Form("Busy violation distances link "
                           "%i:%i:%i", mLayer, mStave, mLink),
                      50,0,50);

  h3->GetXaxis()->SetTitle("Busy violation trigger distance");
  h3->GetYaxis()->SetTitle("Counts");

  for(auto busyv_dist_it = mBusyVTriggerDistances.begin();
      busyv_dist_it != mBusyVTriggerDistances.end();
      busyv_dist_it++)
  {
    h3->Fill(*busyv_dist_it);
  }

  gStyle->SetOptStat("men");
  h3->SetStats(true);
  h3->Write();


  //----------------------------------------------------------------------------
  // Plot busy violation trigger sequence distribution
  //----------------------------------------------------------------------------
  TH1D *h4 = new TH1D("h_busyv_sequence",
                      Form("Busy violation sequences link "
                           "%i:%i:%i", mLayer, mStave, mLink),
                      50,0,50);

  h4->GetXaxis()->SetTitle("Busy violation trigger sequence length");
  h4->GetYaxis()->SetTitle("Counts");

  for(auto busyv_dist_it = mBusyVTriggerSequences.begin();
      busyv_dist_it != mBusyVTriggerSequences.end();
      busyv_dist_it++)
  {
    h4->Fill(*busyv_dist_it);
  }

  gStyle->SetOptStat("men");
  h4->SetStats(true);
  h4->Write();


  //----------------------------------------------------------------------------
  // Plot link utilization histogram (counts of data word types)
  //----------------------------------------------------------------------------
  unsigned int num_fields = 0;

  // mProtocolUtilization has fields for byte counts for each data word,
  // as well as counts of each data word type (not taking size of data word into account).
  // Find out how many fields of the "count" type there are first
  for(auto prot_util_it = mProtUtilIndex.begin();
      prot_util_it != mProtUtilIndex.end();
      prot_util_it++)
  {
    std::string field_name = prot_util_it->second;
    if(field_name.find("count") != std::string::npos) {
      // Ignore COMMA, UNKNOWN and IDLE_TOTAL fields
      if(field_name.find("COMMA") == std::string::npos &&
         field_name.find("IDLE_TOTAL") == std::string::npos &&
         field_name.find("UNKNOWN") == std::string::npos) {
        num_fields++;
      }
    }
  }

  TH1D *h5 = new TH1D("h_prot_util_counts",
                      Form("Protocol utilization (counts) link %i:%i:%i", mLayer, mStave, mLink),
                      num_fields,
                      0.5,
                      num_fields+0.5);

  h5->GetYaxis()->SetTitle("Counts");

  unsigned int bin_index = 0;

  for(auto prot_util_it = mProtUtilIndex.begin();
      prot_util_it != mProtUtilIndex.end();
      prot_util_it++)
  {
    //unsigned int bin_index = prot_util_it->first + 1;
    std::string field_name = prot_util_it->second;
    if(field_name.find("count") != std::string::npos) {
      if(field_name.find("COMMA") == std::string::npos &&
         field_name.find("IDLE_TOTAL") == std::string::npos &&
         field_name.find("UNKNOWN") == std::string::npos) {
        // Extract name of field minus " (count)".
        std::string bin_name = field_name.substr(0, field_name.find(" (count)"));
        h5->Fill(bin_index+1, mProtocolUtilization[field_name]);
        h5->GetXaxis()->SetBinLabel(bin_index+1, bin_name.c_str());
        bin_index++;
      }
    }
  }

  // Draw labels on X axis vertically
  //h5->LabelsOption("v", "x");

  h5->SetFillColor(33);
  h5->SetStats(false);
  h5->Draw("BAR1 TEXT00");
  h5->Write();


  //----------------------------------------------------------------------------
  // Plot link utilization histogram (number of bytes per data word type)
  //----------------------------------------------------------------------------
  num_fields = 0;
  // mProtocolUtilization has fields for byte counts for each data word,
  // as well as counts of each data word type (not taking size of data word into account).
  // Find out how many fields of the "bytes" type there are first
  for(auto prot_util_it = mProtUtilIndex.begin();
      prot_util_it != mProtUtilIndex.end();
      prot_util_it++)
  {
    std::string field_name = prot_util_it->second;
    if(field_name.find("bytes") != std::string::npos) {
      // Ignore COMMA, UNKNOWN and IDLE_TOTAL fields
      if(field_name.find("COMMA") == std::string::npos &&
         field_name.find("IDLE_TOTAL") == std::string::npos &&
         field_name.find("UNKNOWN") == std::string::npos) {
        num_fields++;
      }
    }
  }

  TH1D *h6 = new TH1D("h_prot_util_bytes",
                      Form("Protocol utilization (bytes) link %i:%i:%i", mLayer, mStave, mLink),
                      num_fields,
                      0.5,
                      num_fields+0.5);

  h6->GetYaxis()->SetTitle("Bytes");

  bin_index = 0;

  for(auto prot_util_it = mProtUtilIndex.begin();
      prot_util_it != mProtUtilIndex.end();
      prot_util_it++)
  {
    //unsigned int bin_index = prot_util_it->first + 1;
    std::string field_name = prot_util_it->second;
    if(field_name.find("bytes") != std::string::npos) {
      if(field_name.find("COMMA") == std::string::npos &&
         field_name.find("IDLE_TOTAL") == std::string::npos &&
         field_name.find("UNKNOWN") == std::string::npos) {
        // Extract name of field minus " (bytes)".
        std::string bin_name = field_name.substr(0, field_name.find(" (bytes)"));
        h6->Fill(bin_index+1, mProtocolUtilization[field_name]);
        h6->GetXaxis()->SetBinLabel(bin_index+1, bin_name.c_str());
        bin_index++;
      }
    }
  }

  // Draw labels on X axis vertically
  //h6->LabelsOption("v", "x");

  h6->SetFillColor(33);
  h6->SetStats(false);
  h6->Draw("BAR1 TEXT00");
  h6->Write();


  delete h1;
  delete h2;
  delete h3;
  delete h4;
  delete h5;
  delete h6;
}
