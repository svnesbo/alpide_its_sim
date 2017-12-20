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

  h4->SetStats(true);
  h4->Write();


  //----------------------------------------------------------------------------
  // Plot link utilization histogram
  //----------------------------------------------------------------------------
  TH1D *h5 = new TH1D("h_prot_util",
                      Form("Protocol utilization link %i:%i%i", mLayer, mStave, mLink),
                      mProtocolUtilization.size(),
                      0,
                      mProtocolUtilization.size()-1);

  //h5->GetXaxis()->SetTitle("Data word type");
  h5->GetYaxis()->SetTitle("Counts");

  for(auto prot_util_it = mProtUtilIndex.begin();
      prot_util_it != mProtUtilIndex.end();
      prot_util_it++)
  {
    unsigned int bin_index = prot_util_it->first + 1;
    std::string bin_name = prot_util_it->second;
    h5->Fill(bin_index, mProtocolUtilization[bin_name]);
    h5->GetXaxis()->SetBinLabel(bin_index, bin_name.c_str());
  }

  // Draw labels on X axis vertically
  h5->LabelsOption("v", "x");

  //h5->SetStats(true);
  h5->Write();


  delete h1;
  delete h2;
  delete h3;
  delete h4;
  delete h5;
}
