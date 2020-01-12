#include <TStyle.h>
#include <TFile.h>
#include <TTree.h>
#include <TH1F.h>
#include <TH2Poly.h>
#include <TPad.h>
#include <TCanvas.h>
#include <iostream>
#include <fstream>
#include <utility>
#include <vector>
#include "../../src/Detector/Focal/Focal_constants.hpp"
#include "focal_detector_plane.hpp"
#include "read_csv.hpp"


//void plotBusyAndOccupancy(const char *path, double event_rate_ns, bool continuous_mode)
int main(int argc, char** argv)
{
  if(argc != 2) {
    std::cout << "I take one argument: path to simulation run directory" << std::endl;
    exit(0);
  }

  char* path = argv[1];

  std::string root_filename = std::string(path) + "/focal.root";
  TFile *f = new TFile(root_filename.c_str(), "recreate");

  std::string plots_path = std::string(path) + std::string("/plots");
  std::string mkdir_plots_path = "mkdir " + plots_path;
  system(mkdir_plots_path.c_str());

  std::vector<std::map<std::string, unsigned long> > alpide_data;

  alpide_data = read_csv(std::string(path) + std::string("/Alpide_stats.csv"), ';');

  TCanvas *c1 = new TCanvas("c1","c1",1200,800);

  TH2Poly *h1_pixels_avg = new TH2Poly();
  TH2Poly *h1_busy = new TH2Poly();
  TH2Poly *h1_busyv = new TH2Poly();
  TH2Poly *h1_flush = new TH2Poly();
  TH2Poly *h1_frame_efficiency = new TH2Poly();
  TH2Poly *h1_frame_loss = new TH2Poly();
  TH2Poly *h1_data = new TH2Poly();
  TH2Poly *h3_pixels_avg = new TH2Poly();
  TH2Poly *h3_busy = new TH2Poly();
  TH2Poly *h3_busyv = new TH2Poly();
  TH2Poly *h3_flush = new TH2Poly();
  TH2Poly *h3_frame_efficiency = new TH2Poly();
  TH2Poly *h3_frame_loss = new TH2Poly();
  TH2Poly *h3_data = new TH2Poly();

  h1_pixels_avg->SetName("h1_pixels_avg");
  h1_pixels_avg->SetTitle("Average number of pixel hits per frame - Layer S1");
  create_focal_chip_bins(h1_pixels_avg);

  h1_busy->SetName("h1_busy");
  h1_busy->SetTitle("Average number of busy per frame - Layer S1");
  create_focal_chip_bins(h1_busy);

  h1_busyv->SetName("h1_busyv");
  h1_busyv->SetTitle("Average number of busy violation per frame - Layer S1");
  create_focal_chip_bins(h1_busyv);

  h1_flush->SetName("h1_flush");
  h1_flush->SetTitle("Average number of flush incomplete per frame - Layer S1");
  create_focal_chip_bins(h1_flush);

  h1_frame_efficiency->SetName("h1_frame_efficiency");
  h1_frame_efficiency->SetTitle("Frame readout efficiency - Layer S1");
  create_focal_chip_bins(h1_frame_efficiency);

  h1_frame_loss->SetName("h1_frame_loss");
  h1_frame_loss->SetTitle("Frame readout loss - Layer S1");
  create_focal_chip_bins(h1_frame_loss);

  h1_data->SetName("h1_data");
  h1_data->SetTitle("Average data rate - Layer S1");
  create_focal_chip_bins(h1_data);

  h3_pixels_avg->SetName("h3_pixels_avg");
  h3_pixels_avg->SetTitle("Average number of pixel hits per frame - Layer S3");
  create_focal_chip_bins(h3_pixels_avg);

  h3_busy->SetName("h3_busy");
  h3_busy->SetTitle("Average number of busy per frame - Layer S3");
  create_focal_chip_bins(h3_busy);

  h3_busyv->SetName("h3_busyv");
  h3_busyv->SetTitle("Average number of busy violation per frame - Layer S3");
  create_focal_chip_bins(h3_busyv);

  h3_flush->SetName("h3_flush");
  h3_flush->SetTitle("Average number of flush incomplete per frame - Layer S3");
  create_focal_chip_bins(h3_flush);

  h3_frame_efficiency->SetName("h3_frame_efficiency");
  h3_frame_efficiency->SetTitle("Frame readout efficiency - Layer S3");
  create_focal_chip_bins(h3_frame_efficiency);

  h3_frame_loss->SetName("h3_frame_loss");
  h3_frame_loss->SetTitle("Frame readout loss - Layer S3");
  create_focal_chip_bins(h3_frame_loss);

  h3_data->SetName("h3_data");
  h3_data->SetTitle("Average data rate - Layer S3");
  create_focal_chip_bins(h3_data);

  gStyle->SetPalette(1);


  for(unsigned int i = 0; i < alpide_data.size(); i++) {
    unsigned int busy_count = alpide_data[i]["Busy"];
    unsigned int busyv_count = alpide_data[i]["Busy violations"];
    unsigned int flush_count = alpide_data[i]["Flushed Incompletes"];
    unsigned long pixel_hits = alpide_data[i]["Latched pixel hits"];
    unsigned long accepted_trigs = alpide_data[i]["Accepted triggers"];
    unsigned long received_trigs = alpide_data[i]["Received triggers"];

    unsigned int layer = alpide_data[i]["Layer ID"];
    unsigned int chip_id_in_layer = alpide_data[i]["Unique Chip ID"];

    if(layer > 0)
      chip_id_in_layer -= Focal::CHIPS_PER_LAYER;

    unsigned int bin_num = chip_id_in_layer+1;

    double avg_pix_hit_occupancy = (double)pixel_hits / accepted_trigs;
    double frame_readout_efficiency = 1.0 - (busyv_count+flush_count)/(double)received_trigs;

    if(layer == 0) {
      h1_pixels_avg->SetBinContent(bin_num, avg_pix_hit_occupancy);
      h1_busy->SetBinContent(bin_num, busy_count);
      h1_busyv->SetBinContent(bin_num, busyv_count);
      h1_flush->SetBinContent(bin_num, flush_count);
      h1_frame_efficiency->SetBinContent(bin_num, frame_readout_efficiency);
      h1_frame_loss->SetBinContent(bin_num, 1.0-frame_readout_efficiency);
    } else {
      h3_pixels_avg->AddBinContent(bin_num, avg_pix_hit_occupancy);
      h3_busy->SetBinContent(bin_num, busy_count);
      h3_busyv->SetBinContent(bin_num, busyv_count);
      h3_flush->SetBinContent(bin_num, flush_count);
      h3_frame_efficiency->SetBinContent(bin_num, frame_readout_efficiency);
      h3_frame_loss->SetBinContent(bin_num, 1.0-frame_readout_efficiency);
    }
  }

  h1_pixels_avg->SetStats(0);
  h1_busy->SetStats(0);
  h1_busyv->SetStats(0);
  h1_flush->SetStats(0);
  h1_frame_efficiency->SetStats(0);
  h1_frame_loss->SetStats(0);

  h3_pixels_avg->SetStats(0);
  h3_busy->SetStats(0);
  h3_busyv->SetStats(0);
  h3_flush->SetStats(0);
  h3_frame_efficiency->SetStats(0);
  h3_frame_loss->SetStats(0);

  h1_pixels_avg->GetXaxis()->SetTitle("X [mm]");
  h1_busy->GetXaxis()->SetTitle("X [mm]");
  h1_busyv->GetXaxis()->SetTitle("X [mm]");
  h1_flush->GetXaxis()->SetTitle("X [mm]");
  h1_frame_efficiency->GetXaxis()->SetTitle("X [mm]");
  h1_frame_loss->GetXaxis()->SetTitle("X [mm]");

  h3_pixels_avg->GetXaxis()->SetTitle("X [mm]");
  h3_busy->GetXaxis()->SetTitle("X [mm]");
  h3_busyv->GetXaxis()->SetTitle("X [mm]");
  h3_flush->GetXaxis()->SetTitle("X [mm]");
  h3_frame_efficiency->GetXaxis()->SetTitle("X [mm]");
  h3_frame_loss->GetXaxis()->SetTitle("X [mm]");

  h1_pixels_avg->GetYaxis()->SetTitle("Y [mm]");
  h1_busy->GetYaxis()->SetTitle("Y [mm]");
  h1_busyv->GetYaxis()->SetTitle("Y [mm]");
  h1_flush->GetYaxis()->SetTitle("Y [mm]");
  h1_frame_efficiency->GetYaxis()->SetTitle("Y [mm]");
  h1_frame_loss->GetYaxis()->SetTitle("Y [mm]");

  h3_pixels_avg->GetYaxis()->SetTitle("Y [mm]");
  h3_busy->GetYaxis()->SetTitle("Y [mm]");
  h3_busyv->GetYaxis()->SetTitle("Y [mm]");
  h3_flush->GetYaxis()->SetTitle("Y [mm]");
  h3_frame_efficiency->GetYaxis()->SetTitle("Y [mm]");
  h3_frame_loss->GetYaxis()->SetTitle("Y [mm]");

  h1_pixels_avg->GetYaxis()->SetTitleOffset(1.4);
  h1_busy->GetYaxis()->SetTitleOffset(1.4);
  h1_busyv->GetYaxis()->SetTitleOffset(1.4);
  h1_flush->GetYaxis()->SetTitleOffset(1.4);
  h1_frame_efficiency->GetYaxis()->SetTitleOffset(1.4);
  h1_frame_loss->GetYaxis()->SetTitleOffset(1.4);

  h3_pixels_avg->GetYaxis()->SetTitleOffset(1.4);
  h3_busy->GetYaxis()->SetTitleOffset(1.4);
  h3_busyv->GetYaxis()->SetTitleOffset(1.4);
  h3_flush->GetYaxis()->SetTitleOffset(1.4);
  h3_frame_efficiency->GetYaxis()->SetTitleOffset(1.4);
  h3_frame_loss->GetYaxis()->SetTitleOffset(1.4);


  // Drawing and writing to root file
  gStyle->SetPalette(1);
  h1_pixels_avg->Draw("COLZ L");
  h1_pixels_avg->Write();

  gStyle->SetPalette(1);
  h1_busy->Draw("COLZ L");
  h1_busy->Write();

  gStyle->SetPalette(1);
  h1_busyv->Draw("COLZ L");
  h1_busyv->Write();

  gStyle->SetPalette(1);
  h1_flush->Draw("COLZ L");
  h1_flush->Write();

  gStyle->SetPalette(1);
  h1_frame_efficiency->Draw("COLZ L");
  h1_frame_efficiency->Write();

  gStyle->SetPalette(1);
  h1_frame_loss->Draw("COLZ L");
  h1_frame_loss->Write();

  gStyle->SetPalette(1);
  h3_pixels_avg->Draw("COLZ L");
  h3_pixels_avg->Write();

  gStyle->SetPalette(1);
  h3_busy->Draw("COLZ L");
  h3_busy->Write();

  gStyle->SetPalette(1);
  h3_busyv->Draw("COLZ L");
  h3_busyv->Write();

  gStyle->SetPalette(1);
  h3_flush->Draw("COLZ L");
  h3_flush->Write();

  gStyle->SetPalette(1);
  h3_frame_efficiency->Draw("COLZ L");
  h3_frame_efficiency->Write();

  gStyle->SetPalette(1);
  h3_frame_loss->Draw("COLZ L");
  h3_frame_loss->Write();

  delete c1;
  delete f;
}
