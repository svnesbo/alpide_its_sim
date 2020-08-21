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
#include "../get_num_events.hpp"
#include "../src/Settings/Settings.hpp"
#include "../../src/Detector/Focal/Focal_constants.hpp"
#include "../../src/Detector/ITS/ITS_constants.hpp"
#include "focal_detector_plane.hpp"
#include "read_csv.hpp"

const unsigned int NUM_BINS_RADIUS_PLOTS = 15;
const double CHIP_SIZE_X_MM = 30.0;

const unsigned int C_ALPIDE_CHIP_HEADER_BYTES = 2;
const unsigned int C_ALPIDE_CHIP_TRAILER_BYTES = 1;
const unsigned int C_ALPIDE_CHIP_EMPTY_FRAME_BYTES = 2;
const unsigned int C_ALPIDE_REGION_HEADER_BYTES = 1;
const unsigned int C_ALPIDE_DATA_SHORT_BYTES = 2;
const unsigned int C_ALPIDE_DATA_LONG_BYTES = 3;
const unsigned int C_ALPIDE_BUSY_ON_BYTES = 1;
const unsigned int C_ALPIDE_BUSY_OFF_BYTES = 1;

double calculate_data_rate(std::map<std::string, unsigned long>& alpide_data_entry,
                           unsigned long sim_time_ns)
{
  double data_bytes = 0;

  auto chip_header_it = alpide_data_entry.find("ALPIDE_CHIP_HEADER");
  auto chip_trailer_it = alpide_data_entry.find("ALPIDE_CHIP_TRAILER");
  auto chip_empty_frame_it = alpide_data_entry.find("ALPIDE_CHIP_EMPTY_FRAME");
  auto region_header_it = alpide_data_entry.find("ALPIDE_REGION_HEADER");
  auto data_short_it = alpide_data_entry.find("ALPIDE_DATA_SHORT");
  auto data_long_it = alpide_data_entry.find("ALPIDE_DATA_LONG");
  auto busy_on_it = alpide_data_entry.find("ALPIDE_BUSY_ON");
  auto busy_off_it = alpide_data_entry.find("ALPIDE_BUSY_OFF");

  if(chip_header_it != alpide_data_entry.end()) {
    data_bytes += chip_header_it->second * C_ALPIDE_CHIP_HEADER_BYTES;
  }

  if(chip_trailer_it != alpide_data_entry.end()) {
    data_bytes += chip_trailer_it->second * C_ALPIDE_CHIP_TRAILER_BYTES;
  }

  if(chip_empty_frame_it != alpide_data_entry.end()) {
    data_bytes += chip_empty_frame_it->second * C_ALPIDE_CHIP_EMPTY_FRAME_BYTES;
  }

  if(region_header_it != alpide_data_entry.end()) {
    data_bytes += region_header_it->second * C_ALPIDE_REGION_HEADER_BYTES;
  }

  if(data_short_it != alpide_data_entry.end()) {
    data_bytes += data_short_it->second * C_ALPIDE_DATA_SHORT_BYTES;
  }

  if(data_long_it != alpide_data_entry.end()) {
    data_bytes += data_long_it->second * C_ALPIDE_DATA_LONG_BYTES;
  }

  if(busy_on_it != alpide_data_entry.end()) {
    data_bytes += busy_on_it->second * C_ALPIDE_BUSY_ON_BYTES;
  }

  if(busy_off_it != alpide_data_entry.end()) {
    data_bytes += busy_off_it->second * C_ALPIDE_BUSY_OFF_BYTES;
  }

  double data_megabits = (8*data_bytes) / (1024*1024);

  double sim_time_seconds = sim_time_ns / 1.0E9;

  double data_rate_mbps = data_megabits / sim_time_seconds;

  return data_rate_mbps;
}


///@brief Check if a specific chip ID was included in the simulation
///@param global_chip_id Global chip ID to check
///@param staves_per_quadrant Number of staves simulated per quadrant in the simulation
///@return True if chip ID is included in simulation
bool is_chip_id_included_in_sim(unsigned int global_chip_id, unsigned int staves_per_quadrant)
{
  unsigned int chip_id_in_layer = global_chip_id % Focal::CHIPS_PER_LAYER;
  unsigned int stave_id = chip_id_in_layer / Focal::CHIPS_PER_STAVE;
  unsigned int stave_num_in_quadrant = stave_id % Focal::STAVES_PER_QUADRANT;

  if(stave_num_in_quadrant < staves_per_quadrant)
    return true;
  else
    return false;
}


///@brief Determine if an entry in the alpide stats csv file is
///       for a chip in outer barrel master mode
bool is_outer_barrel_master(std::map<std::string, unsigned long>& alpide_data_entry)
{
  unsigned int stave_num_in_quadrant = alpide_data_entry["Stave ID"] % Focal::STAVES_PER_QUADRANT;
  unsigned int module_id = alpide_data_entry["Module ID"];
  unsigned int module_chip_id = alpide_data_entry["Local Chip ID"];

  if(stave_num_in_quadrant < Focal::INNER_STAVES_PER_QUADRANT &&
     module_id > 0 &&
     module_chip_id == 0)
  {
    // Outer barrel master chip in Focal Inner Stave
    return true;
  }
  else if(stave_num_in_quadrant >= Focal::INNER_STAVES_PER_QUADRANT &&
          module_chip_id == 0)
  {
    // Outer barrel master chip in Focal Outer Stave
    return true;
  } else {
    // IB chip or OB slave chip
    return false;
  }
}


unsigned int get_ob_master_busy_count(const std::vector<std::map<std::string, unsigned long> >& alpide_data,
                                      unsigned int ob_master_idx)
{
  unsigned int stave_num_in_quadrant = alpide_data[ob_master_idx].at("Stave ID") % Focal::STAVES_PER_QUADRANT;
  unsigned int num_ob_chips;

  if(stave_num_in_quadrant < Focal::INNER_STAVES_PER_QUADRANT)
    num_ob_chips = ITS::CHIPS_PER_HALF_MODULE;
  else
    num_ob_chips = Focal::CHIPS_PER_FOCAL_OB_MODULE;

  if(alpide_data.size() < ob_master_idx+num_ob_chips) {
    std::cerr << "Error: too few entries left to fix busy count for OB master " << std::endl;
    exit(-1);
  }

  // Just average it instead of subtracting the number from the slaves
  // Because the sum of busy slaves can be larger than the number of busys
  // sent by the master chip, since two slaves that are busy at the same time
  // are only counted once by the master...
  int ob_master_busy_count = alpide_data[ob_master_idx].at("Busy") / num_ob_chips;

  /* for(unsigned int slave = 1; slave < num_ob_chips; slave++) { */
  /*   ob_master_busy_count -= alpide_data[ob_master_idx+slave].at("Busy"); */
  /* } */

  /* if(ob_master_busy_count < 0) */
  /*   ob_master_busy_count = 0; */

  return ob_master_busy_count;
}


TH1F* create_radius_plot(unsigned int staves_per_quadrant,
                         const TH2Poly* th2,
                         const char* name,
                         const char* title,
                         const char* y_title,
                         const std::string operation="sum")
{
  TH1F *h_radius = new TH1F(name, title, NUM_BINS_RADIUS_PLOTS, 0, NUM_BINS_RADIUS_PLOTS*CHIP_SIZE_X_MM);

  // Number of times we've filled a specific radius bin.. use for averaging later
  std::map<unsigned int, unsigned int> radius_bin_fill_counts;

  for(int bin_num = 1; bin_num <= th2->GetNumberOfBins(); bin_num++) {
    // Skip chips that are not included in simulation
    if(is_chip_id_included_in_sim(bin_num-1, staves_per_quadrant) == false)
      continue;

    // bin numbers start at 1...
    unsigned int radius_bin = bin_number_to_radius_bin(bin_num) + 1;

    if(operation == "sum" || operation == "avg") {
      h_radius->AddBinContent(radius_bin,
                              th2->GetBinContent(bin_num));

      radius_bin_fill_counts[radius_bin]++;
    }
    else if(operation == "max") {
      if(th2->GetBinContent(bin_num) > h_radius->GetBinContent(radius_bin))
        h_radius->SetBinContent(radius_bin, th2->GetBinContent(bin_num));
    }
    else {
      std::cerr << "Error: unknown operation " << operation << std::endl;
      exit(-1);
    }
  }

  if(operation == "avg") {
    for(unsigned int radius_bin = 1; radius_bin <= NUM_BINS_RADIUS_PLOTS; radius_bin++) {
      unsigned int N = radius_bin_fill_counts[radius_bin];

      double average = (double) h_radius->GetBinContent(radius_bin) / N;
      h_radius->SetBinContent(radius_bin, average);
    }
  }

  h_radius->GetXaxis()->SetTitle("X [mm]");
  h_radius->GetYaxis()->SetTitle(y_title);
  h_radius->GetYaxis()->SetTitleOffset(1.4);

  return h_radius;
}


//void plotBusyAndOccupancy(const char *path, double event_rate_ns, bool continuous_mode)
int main(int argc, char** argv)
{
  if(argc != 2) {
    std::cout << "I take one argument: path to simulation run directory" << std::endl;
    exit(0);
  }

  char* path = argv[1];

  QString settings_file_path = QString(path) + "/settings.txt";
  QSettings *sim_settings = new QSettings(settings_file_path, QSettings::IniFormat);

  std::string root_filename = std::string(path) + "/focal.root";
  TFile *f = new TFile(root_filename.c_str(), "recreate");

  std::string plots_path = std::string(path) + std::string("/plots");
  std::string mkdir_plots_path = "mkdir " + plots_path;
  system(mkdir_plots_path.c_str());

  std::vector<std::map<std::string, unsigned long> > alpide_data;

  alpide_data = read_csv(std::string(path) + std::string("/Alpide_stats.csv"), ';');

  TCanvas *c1 = new TCanvas("c1","c1",1200,800);

  TH2Poly *h1_pixels_avg = new TH2Poly();
  TH2Poly *h1_busy_avg = new TH2Poly();
  TH2Poly *h1_busyv_avg = new TH2Poly();
  TH2Poly *h1_flush_avg = new TH2Poly();
  TH2Poly *h1_frame_efficiency = new TH2Poly();
  TH2Poly *h1_frame_loss = new TH2Poly();
  TH2Poly *h1_data = new TH2Poly();
  TH2Poly *h3_pixels_avg = new TH2Poly();
  TH2Poly *h3_busy_avg = new TH2Poly();
  TH2Poly *h3_busyv_avg = new TH2Poly();
  TH2Poly *h3_flush_avg = new TH2Poly();
  TH2Poly *h3_frame_efficiency = new TH2Poly();
  TH2Poly *h3_frame_loss = new TH2Poly();
  TH2Poly *h3_data = new TH2Poly();

  h1_pixels_avg->SetName("h1_pixels_avg");
  h1_pixels_avg->SetTitle("Average number of pixel hits per frame - Layer S1");
  create_focal_chip_bins(h1_pixels_avg);

  h1_busy_avg->SetName("h1_busy_avg");
  h1_busy_avg->SetTitle("Average number of busy per frame - Layer S1");
  create_focal_chip_bins(h1_busy_avg);

  h1_busyv_avg->SetName("h1_busyv_avg");
  h1_busyv_avg->SetTitle("Average number of busy violation per frame - Layer S1");
  create_focal_chip_bins(h1_busyv_avg);

  h1_flush_avg->SetName("h1_flush_avg");
  h1_flush_avg->SetTitle("Average number of flush incomplete per frame - Layer S1");
  create_focal_chip_bins(h1_flush_avg);

  h1_frame_efficiency->SetName("h1_frame_efficiency");
  h1_frame_efficiency->SetTitle("Frame readout efficiency - Layer S1");
  create_focal_chip_bins(h1_frame_efficiency);

  h1_frame_loss->SetName("h1_frame_loss");
  h1_frame_loss->SetTitle("Frame readout loss - Layer S1");
  create_focal_chip_bins(h1_frame_loss);

  h1_data->SetName("h1_data");
  h1_data->SetTitle("Average data rate [Mbps] - Layer S1");
  create_focal_chip_bins(h1_data);

  h3_pixels_avg->SetName("h3_pixels_avg");
  h3_pixels_avg->SetTitle("Average number of pixel hits per frame - Layer S3");
  create_focal_chip_bins(h3_pixels_avg);

  h3_busy_avg->SetName("h3_busy_avg");
  h3_busy_avg->SetTitle("Average number of busy per frame - Layer S3");
  create_focal_chip_bins(h3_busy_avg);

  h3_busyv_avg->SetName("h3_busyv_avg");
  h3_busyv_avg->SetTitle("Average number of busy violation per frame - Layer S3");
  create_focal_chip_bins(h3_busyv_avg);

  h3_flush_avg->SetName("h3_flush_avg");
  h3_flush_avg->SetTitle("Average number of flush incomplete per frame - Layer S3");
  create_focal_chip_bins(h3_flush_avg);

  h3_frame_efficiency->SetName("h3_frame_efficiency");
  h3_frame_efficiency->SetTitle("Frame readout efficiency - Layer S3");
  create_focal_chip_bins(h3_frame_efficiency);

  h3_frame_loss->SetName("h3_frame_loss");
  h3_frame_loss->SetTitle("Frame readout loss - Layer S3");
  create_focal_chip_bins(h3_frame_loss);

  h3_data->SetName("h3_data");
  h3_data->SetTitle("Average data rate [Mbps] - Layer S3");
  create_focal_chip_bins(h3_data);

  gStyle->SetPalette(1);

  unsigned int event_rate_ns = sim_settings->value("event/average_event_rate_ns").toUInt();
  unsigned long num_physics_events = get_num_triggered_events_simulated(path);
  unsigned long sim_time_ns = event_rate_ns*num_physics_events;

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

    if(is_outer_barrel_master(alpide_data[i])) {
      // Busy count in the CSV file contains sum of busy from all slaves + master
      // for the OB master chip. This function subtracts the busy counts for the slaves
      busy_count = get_ob_master_busy_count(alpide_data, i);
    }

    double avg_pix_hit_occupancy = (double)pixel_hits / accepted_trigs;
    double avg_busy_count = (double)busy_count / received_trigs;
    double avg_busyv_count = (double)busyv_count / received_trigs;
    double avg_flush_count = (double)flush_count / accepted_trigs;
    double frame_readout_efficiency = 1.0 - (busyv_count+flush_count)/(double)received_trigs;

    double data_rate_mbps = calculate_data_rate(alpide_data[i], sim_time_ns);

    if(layer == 0) {
      h1_pixels_avg->SetBinContent(bin_num, avg_pix_hit_occupancy);
      h1_busy_avg->SetBinContent(bin_num, avg_busy_count);
      h1_busyv_avg->SetBinContent(bin_num, avg_busyv_count);
      h1_flush_avg->SetBinContent(bin_num, avg_flush_count);
      h1_frame_efficiency->SetBinContent(bin_num, frame_readout_efficiency);
      h1_frame_loss->SetBinContent(bin_num, 1.0-frame_readout_efficiency);
      h1_data->SetBinContent(bin_num, data_rate_mbps);
    } else {
      h3_pixels_avg->SetBinContent(bin_num, avg_pix_hit_occupancy);
      h3_busy_avg->SetBinContent(bin_num, avg_busy_count);
      h3_busyv_avg->SetBinContent(bin_num, avg_busyv_count);
      h3_flush_avg->SetBinContent(bin_num, avg_flush_count);
      h3_frame_efficiency->SetBinContent(bin_num, frame_readout_efficiency);
      h3_frame_loss->SetBinContent(bin_num, 1.0-frame_readout_efficiency);
      h3_data->SetBinContent(bin_num, data_rate_mbps);
    }
  }

  c1->SetRightMargin(0.2);
  c1->Update();

  h1_pixels_avg->SetStats(0);
  h1_busy_avg->SetStats(0);
  h1_busyv_avg->SetStats(0);
  h1_flush_avg->SetStats(0);
  h1_frame_efficiency->SetStats(0);
  h1_frame_loss->SetStats(0);
  h1_data->SetStats(0);

  h3_pixels_avg->SetStats(0);
  h3_busy_avg->SetStats(0);
  h3_busyv_avg->SetStats(0);
  h3_flush_avg->SetStats(0);
  h3_frame_efficiency->SetStats(0);
  h3_frame_loss->SetStats(0);
  h3_data->SetStats(0);

  h1_pixels_avg->GetXaxis()->SetTitle("X [mm]");
  h1_busy_avg->GetXaxis()->SetTitle("X [mm]");
  h1_busyv_avg->GetXaxis()->SetTitle("X [mm]");
  h1_flush_avg->GetXaxis()->SetTitle("X [mm]");
  h1_frame_efficiency->GetXaxis()->SetTitle("X [mm]");
  h1_frame_loss->GetXaxis()->SetTitle("X [mm]");
  h1_data->GetXaxis()->SetTitle("X [mm]");

  h3_pixels_avg->GetXaxis()->SetTitle("X [mm]");
  h3_busy_avg->GetXaxis()->SetTitle("X [mm]");
  h3_busyv_avg->GetXaxis()->SetTitle("X [mm]");
  h3_flush_avg->GetXaxis()->SetTitle("X [mm]");
  h3_frame_efficiency->GetXaxis()->SetTitle("X [mm]");
  h3_frame_loss->GetXaxis()->SetTitle("X [mm]");
  h3_data->GetXaxis()->SetTitle("X [mm]");

  h1_pixels_avg->GetYaxis()->SetTitle("Y [mm]");
  h1_busy_avg->GetYaxis()->SetTitle("Y [mm]");
  h1_busyv_avg->GetYaxis()->SetTitle("Y [mm]");
  h1_flush_avg->GetYaxis()->SetTitle("Y [mm]");
  h1_frame_efficiency->GetYaxis()->SetTitle("Y [mm]");
  h1_frame_loss->GetYaxis()->SetTitle("Y [mm]");
  h1_data->GetYaxis()->SetTitle("Y [mm]");

  h3_pixels_avg->GetYaxis()->SetTitle("Y [mm]");
  h3_busy_avg->GetYaxis()->SetTitle("Y [mm]");
  h3_busyv_avg->GetYaxis()->SetTitle("Y [mm]");
  h3_flush_avg->GetYaxis()->SetTitle("Y [mm]");
  h3_frame_efficiency->GetYaxis()->SetTitle("Y [mm]");
  h3_frame_loss->GetYaxis()->SetTitle("Y [mm]");
  h3_data->GetYaxis()->SetTitle("Y [mm]");

  h1_pixels_avg->GetYaxis()->SetTitleOffset(1.4);
  h1_busy_avg->GetYaxis()->SetTitleOffset(1.4);
  h1_busyv_avg->GetYaxis()->SetTitleOffset(1.4);
  h1_flush_avg->GetYaxis()->SetTitleOffset(1.4);
  h1_frame_efficiency->GetYaxis()->SetTitleOffset(1.4);
  h1_frame_loss->GetYaxis()->SetTitleOffset(1.4);
  h1_data->GetYaxis()->SetTitleOffset(1.4);

  h3_pixels_avg->GetYaxis()->SetTitleOffset(1.4);
  h3_busy_avg->GetYaxis()->SetTitleOffset(1.4);
  h3_busyv_avg->GetYaxis()->SetTitleOffset(1.4);
  h3_flush_avg->GetYaxis()->SetTitleOffset(1.4);
  h3_frame_efficiency->GetYaxis()->SetTitleOffset(1.4);
  h3_frame_loss->GetYaxis()->SetTitleOffset(1.4);
  h3_data->GetYaxis()->SetTitleOffset(1.4);


  // Drawing and writing to root file
  gStyle->SetPalette(1);
  h1_pixels_avg->Draw("COLZ L");
  h1_pixels_avg->Write();
  c1->Print(Form("%s/h1_pixels_avg.png", plots_path.c_str()));
  c1->Print(Form("%s/h1_pixels_avg.pdf", plots_path.c_str()));

  gStyle->SetPalette(1);
  h1_busy_avg->Draw("COLZ L");
  h1_busy_avg->Write();
  c1->Print(Form("%s/h1_busy_avg.png", plots_path.c_str()));
  c1->Print(Form("%s/h1_busy_avg.pdf", plots_path.c_str()));

  gStyle->SetPalette(1);
  h1_busyv_avg->Draw("COLZ L");
  h1_busyv_avg->Write();
  c1->Print(Form("%s/h1_busyv_avg.png", plots_path.c_str()));
  c1->Print(Form("%s/h1_busyv_avg.pdf", plots_path.c_str()));

  gStyle->SetPalette(1);
  h1_flush_avg->Draw("COLZ L");
  h1_flush_avg->Write();
  c1->Print(Form("%s/h1_flush_avg.png", plots_path.c_str()));
  c1->Print(Form("%s/h1_flush_avg.pdf", plots_path.c_str()));

  gStyle->SetPalette(1);
  h1_frame_efficiency->Draw("COLZ L");
  h1_frame_efficiency->Write();
  c1->Print(Form("%s/h1_frame_efficiency.png", plots_path.c_str()));
  c1->Print(Form("%s/h1_frame_efficiency.pdf", plots_path.c_str()));

  gStyle->SetPalette(1);
  h1_frame_loss->Draw("COLZ L");
  h1_frame_loss->Write();
  c1->Print(Form("%s/h1_frame_loss.png", plots_path.c_str()));
  c1->Print(Form("%s/h1_frame_loss.pdf", plots_path.c_str()));

  gStyle->SetPalette(1);
  h1_data->Draw("COLZ L");
  h1_data->Write();
  c1->Print(Form("%s/h1_data.png", plots_path.c_str()));
  c1->Print(Form("%s/h1_data.pdf", plots_path.c_str()));

  gStyle->SetPalette(1);
  h3_pixels_avg->Draw("COLZ L");
  h3_pixels_avg->Write();
  c1->Print(Form("%s/h3_pixels_avg.png", plots_path.c_str()));
  c1->Print(Form("%s/h3_pixels_avg.pdf", plots_path.c_str()));

  gStyle->SetPalette(1);
  h3_busy_avg->Draw("COLZ L");
  h3_busy_avg->Write();
  c1->Print(Form("%s/h3_busy_avg.png", plots_path.c_str()));
  c1->Print(Form("%s/h3_busy_avg.pdf", plots_path.c_str()));

  gStyle->SetPalette(1);
  h3_busyv_avg->Draw("COLZ L");
  h3_busyv_avg->Write();
  c1->Print(Form("%s/h3_busyv_avg.png", plots_path.c_str()));
  c1->Print(Form("%s/h3_busyv_avg.pdf", plots_path.c_str()));

  gStyle->SetPalette(1);
  h3_flush_avg->Draw("COLZ L");
  h3_flush_avg->Write();
  c1->Print(Form("%s/h3_flush_avg.png", plots_path.c_str()));
  c1->Print(Form("%s/h3_flush_avg.pdf", plots_path.c_str()));

  gStyle->SetPalette(1);
  h3_frame_efficiency->Draw("COLZ L");
  h3_frame_efficiency->Write();
  c1->Print(Form("%s/h3_frame_efficiency.png", plots_path.c_str()));
  c1->Print(Form("%s/h3_frame_efficiency.pdf", plots_path.c_str()));

  gStyle->SetPalette(1);
  h3_frame_loss->Draw("COLZ L");
  h3_frame_loss->Write();
  c1->Print(Form("%s/h3_frame_loss.png", plots_path.c_str()));
  c1->Print(Form("%s/h3_frame_loss.pdf", plots_path.c_str()));

  gStyle->SetPalette(1);
  h3_data->Draw("COLZ L");
  h3_data->Write();
  c1->Print(Form("%s/h3_data.png", plots_path.c_str()));
  c1->Print(Form("%s/h3_data.pdf", plots_path.c_str()));


  // ---------------------------------------------------------------------------
  // Make plots versus radius/distance from center of detector plane
  // ---------------------------------------------------------------------------
  unsigned int staves_per_quadrant = sim_settings->value("focal/staves_per_quadrant").toUInt();

  TH1F* h1_data_radius = create_radius_plot(staves_per_quadrant,
                                            h1_data, "h1_data_radius",
                                            "Average data rate per chip - Layer S1",
                                            "Data rate [Mbps]", "avg");
  h1_data_radius->Draw("");
  h1_data_radius->Write();
  c1->Print(Form("%s/h1_data_radius.png", plots_path.c_str()));
  c1->Print(Form("%s/h1_data_radius.pdf", plots_path.c_str()));


  TH1F* h3_data_radius = create_radius_plot(staves_per_quadrant,
                                            h3_data, "h3_data_radius",
                                            "Average data rate per chip - Layer S3",
                                            "Data rate [Mbps]", "avg");
  h3_data_radius->Draw("");
  h3_data_radius->Write();
  c1->Print(Form("%s/h3_data_radius.png", plots_path.c_str()));
  c1->Print(Form("%s/h3_data_radius.pdf", plots_path.c_str()));


  TH1F* h1_pixels_avg_radius = create_radius_plot(staves_per_quadrant,
                                                  h1_pixels_avg, "h1_pixels_avg_radius",
                                                  "Average number of pixel hits per frame per chip - Layer S1",
                                                  "Pixel hits", "avg");
  h1_pixels_avg_radius->Draw("");
  h1_pixels_avg_radius->Write();
  c1->Print(Form("%s/h1_pixels_avg_radius.png", plots_path.c_str()));
  c1->Print(Form("%s/h1_pixels_avg_radius.pdf", plots_path.c_str()));


  TH1F* h3_pixels_avg_radius = create_radius_plot(staves_per_quadrant,
                                                  h3_pixels_avg, "h3_pixels_avg_radius",
                                                  "Average number of pixel hits per frame per chip - Layer S3",
                                                  "Pixel hits", "avg");
  h3_pixels_avg_radius->Draw("");
  h3_pixels_avg_radius->Write();
  c1->Print(Form("%s/h3_pixels_avg_radius.png", plots_path.c_str()));
  c1->Print(Form("%s/h3_pixels_avg_radius.pdf", plots_path.c_str()));


  TH1F* h1_busy_avg_radius = create_radius_plot(staves_per_quadrant,
                                                h1_busy_avg, "h1_busy_avg_radius",
                                                "Average number of busy per frame per chip - Layer S1",
                                                "Busy per frame", "avg");
  h1_busy_avg_radius->Draw("");
  h1_busy_avg_radius->Write();
  c1->Print(Form("%s/h1_busy_avg_radius.png", plots_path.c_str()));
  c1->Print(Form("%s/h1_busy_avg_radius.pdf", plots_path.c_str()));


  TH1F* h3_busy_avg_radius = create_radius_plot(staves_per_quadrant,
                                                h3_busy_avg, "h3_busy_avg_radius",
                                                "Average number of busy per frame per chip - Layer S3",
                                                "Busy per frame", "avg");
  h3_busy_avg_radius->Draw("");
  h3_busy_avg_radius->Write();
  c1->Print(Form("%s/h3_busy_avg_radius.png", plots_path.c_str()));
  c1->Print(Form("%s/h3_busy_avg_radius.pdf", plots_path.c_str()));


  TH1F* h1_busyv_avg_radius = create_radius_plot(staves_per_quadrant,
                                                 h1_busyv_avg, "h1_busyv_avg_radius",
                                                "Average number of busy violations per frame per chip - Layer S1",
                                                "Busy violations per frame", "avg");
  h1_busyv_avg_radius->Draw("");
  h1_busyv_avg_radius->Write();
  c1->Print(Form("%s/h1_busyv_avg_radius.png", plots_path.c_str()));
  c1->Print(Form("%s/h1_busyv_avg_radius.pdf", plots_path.c_str()));


  TH1F* h3_busyv_avg_radius = create_radius_plot(staves_per_quadrant,
                                                 h3_busyv_avg, "h3_busyv_avg_radius",
                                                "Average number of busy violations per frame per chip - Layer S3",
                                                "Busy violations per frame", "avg");
  h3_busyv_avg_radius->Draw("");
  h3_busyv_avg_radius->Write();
  c1->Print(Form("%s/h3_busyv_avg_radius.png", plots_path.c_str()));
  c1->Print(Form("%s/h3_busyv_avg_radius.pdf", plots_path.c_str()));


  TH1F* h1_flush_avg_radius = create_radius_plot(staves_per_quadrant,
                                                 h1_flush_avg, "h1_flush_avg_radius",
                                                 "Average number of flush incompl. per frame per chip - Layer S1",
                                                 "Flush incompl. per frame", "avg");
  h1_flush_avg_radius->Draw("");
  h1_flush_avg_radius->Write();
  c1->Print(Form("%s/h1_flush_avg_radius.png", plots_path.c_str()));
  c1->Print(Form("%s/h1_flush_avg_radius.pdf", plots_path.c_str()));


  TH1F* h3_flush_avg_radius = create_radius_plot(staves_per_quadrant,
                                                 h3_flush_avg, "h3_flush_avg_radius",
                                                 "Average number of flush incompl. per frame per chip - Layer S3",
                                                 "Flush incompl. per frame", "avg");
  h3_flush_avg_radius->Draw("");
  h3_flush_avg_radius->Write();
  c1->Print(Form("%s/h3_flush_avg_radius.png", plots_path.c_str()));
  c1->Print(Form("%s/h3_flush_avg_radius.pdf", plots_path.c_str()));


  TH1F* h1_frame_efficiency_radius = create_radius_plot(staves_per_quadrant,
                                                        h1_frame_efficiency, "h1_frame_efficiency_radius",
                                                        "Frame readout efficiency - Layer S1",
                                                        "Efficiency", "avg");
  h1_frame_efficiency_radius->Draw("");
  h1_frame_efficiency_radius->Write();
  c1->Print(Form("%s/h1_frame_efficiency_radius.png", plots_path.c_str()));
  c1->Print(Form("%s/h1_frame_efficiency_radius.pdf", plots_path.c_str()));


  TH1F* h3_frame_efficiency_radius = create_radius_plot(staves_per_quadrant,
                                                        h3_frame_efficiency, "h3_frame_efficiency_radius",
                                                        "Frame readout efficiency - Layer S3",
                                                        "Efficiency", "avg");
  h3_frame_efficiency_radius->Draw("");
  h3_frame_efficiency_radius->Write();
  c1->Print(Form("%s/h3_frame_efficiency_radius.png", plots_path.c_str()));
  c1->Print(Form("%s/h3_frame_efficiency_radius.pdf", plots_path.c_str()));


  TH1F* h1_frame_loss_radius = create_radius_plot(staves_per_quadrant,
                                                  h1_frame_loss, "h1_frame_loss_radius",
                                                 "Frame readout loss - Layer S1",
                                                 "Loss", "avg");
  h1_frame_loss_radius->Draw("");
  h1_frame_loss_radius->Write();
  c1->Print(Form("%s/h1_frame_loss_radius.png", plots_path.c_str()));
  c1->Print(Form("%s/h1_frame_loss_radius.pdf", plots_path.c_str()));


  TH1F* h3_frame_loss_radius = create_radius_plot(staves_per_quadrant,
                                                  h3_frame_loss, "h3_frame_loss_radius",
                                                  "Frame readout loss - Layer S3",
                                                  "Loss", "avg");
  h3_frame_loss_radius->Draw("");
  h3_frame_loss_radius->Write();
  c1->Print(Form("%s/h3_frame_loss_radius.png", plots_path.c_str()));
  c1->Print(Form("%s/h3_frame_loss_radius.pdf", plots_path.c_str()));

  delete c1;
  delete f;
}
