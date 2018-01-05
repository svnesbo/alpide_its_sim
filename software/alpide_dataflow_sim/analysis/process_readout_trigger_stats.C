#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstring>
#include <string>
#include <vector>
#include <TFile.h>
#include <TCanvas.h>
#include <TROOT.h>
//#include <TApplication.h>
#include <TH1F.h>
#include <TH1I.h>
#include "ReadoutUnitStats.hpp"
#include "ITSLayerStats.hpp"

const std::string csv_delim(";");

const float chip_width_cm = 3.0;
const float chip_height_cm = 1.5;

std::vector<uint64_t> process_event_data(std::string sim_run_data_path,
                                         bool create_png, bool create_pdf);

//enum TrigAction {TRIGGER_SENT, TRIGGER_NOT_SENT_BUSY, TRIGGER_FILTERED};

int process_readout_trigger_stats(const char* sim_run_data_path,
                                  bool create_png,
                                  bool create_pdf)
{
  gROOT->SetBatch(kTRUE);

  std::vector<uint64_t> event_time_vec = process_event_data(sim_run_data_path,
                                                            create_png, create_pdf);


  std::string root_filename = sim_run_data_path + std::string("/busy_data.root");

  // Create/recreate root file
  TFile *f = new TFile(root_filename.c_str(), "recreate");


  /*
  // Indexing: [layer][stave/RU][link ID][trigger ID]
  std::vector<std::vector<std::vector<std::vector<TrigAction>>>> trigger_stats;

  // Map key: trigger ID, vector index: layer, elements in vector: coverage
  std::map<std::vector<double>> trig_coverage_per_layer;

  // Map key: trigger ID, value: coverage
  std::map<uint64_t, double> trig_coverage_detector;

  // Map key: trigger ID, value: number of layers with mismatch for this trigger id
  std::map<uint64_t, uint32_t> trig_filter_mismatch;
  */

  //ReadoutUnitStats RU(0, 0, sim_run_data_path);
  //ITSLayerStats ITS_layer(0, 12, sim_run_data_path);
  ITSLayerStats ITS_layer(0, 1, sim_run_data_path, create_png, create_pdf);


  delete f;

  /*

  // Todo: Check that the sim_run_data_path directory actually exists

  system("mkdir png");
  system("mkdir pdf");

  TFile *f = new TFile("RU_trigger_stats.root", "recreate");

  size_t csv_file_base_path_start = csv_filename_str.rfind("/");



  // 1. Open settings file
  // 2. Get number of staves and layers from settings file
  // 3. Create DetectorStats object with those parameters






  // Go through all the RU_<layer>_<stave>_Trigger_stats.csv files,
  // parse them and construct vector map of trigger stats for all
  // RUs in all layers. One RU per stave.
  for(int layer = 0; layer < 7; layer++) {
    bool last_stave_in_layer = false;
    int stave_num = 0;

    while(last_stave_in_layer == false) {
      std::stringstream ss;
      ss << sim_run_data_path << "/";
      ss << "RU_" << layer << "_" << stave_num << "_Trigger_stats.csv";

      std::ifstream RU_trig_stats_file(ss.str().c_str());

      if(!RU_trig_stats_file.is_open()) {
        std::cout << "Could not open file: " << ss.str();
        std::cout << ". Assuming we reached last RU in this layer.";
        std::cout << std::endl;
        last_stave_in_layer = false;
      } else {
        std::map<int64_t, TrigAction> RU_trig_stats = getTriggerActions(RU_trig_stats_file);
        trigger_stats[layer].push_back(RU_trig_stats);
        stave_num++;
      }
    }
  }


  // Coverage calculation in a layer:
  //
  // N_active_staves = N_staves - N_filtered_staves
  //
  // In principle either all or no staves in a layer (or in the whole detector, in fact) should be filtered.
  //
  // RU coverage:
  // Coverage_RU(trigger_id) = (Number of links with trigger sent) / (Total number of links)
  //
  // Layer coverage:
  // Coverage_layer(trigger_id) = sum(RU coverages) / (Number of RUs in layer)
  //
  // Detector coverage:
  // Coverage_detector(trigger_id) = sum(Layer coverages) / (Number of layers)

  // Process trigger stats data
  for(int layer = 0; layer < trigger_stats.size(); layer++) {
    for(int stave = 0; stave < trigger_stats[layer].size; stave++) {
      std::map<std::vector<double>> trig_coverage_per_layer[layer][]
      for()
    }


  }

  */

}


///@brief Process CSV file with event rate and multiplicity data, generate plots of the data
///       and store them in both png and pdf format, if desired.
///       The first column in the CSV file is expected to be time to the previous event,
///       and the other columns are expected to be multiplicity data. It can have any number
///       of columns after the first column, and a histogram plot of multiplicity will be
///       generated for each column (one can plot multiplicity for each chip for instance).
///@param sim_run_data_path Path to directory with simulation run data
///@param create_png Set to true to save histograms as png files
///@param create_pdf Set to true to save histograms as pdf files
///@return Vector with time to next event for, where index corresponds to event number.
std::vector<uint64_t> process_event_data(std::string sim_run_data_path,
                                         bool create_png, bool create_pdf)
{
  std::vector<uint64_t> event_time_vec;

  std::string csv_filename = sim_run_data_path + "/physics_events_data.csv";
  std::string root_filename = sim_run_data_path + "/physics_events_data.root";
  std::string summary_filename = sim_run_data_path + "/summary.txt";

  TFile *f = new TFile(root_filename.c_str(), "recreate");

  std::ofstream summary_file(summary_filename);
  if(!summary_file.is_open()) {
    std::cerr << "Error opening file " << summary_filename << std::endl;
    exit(-1);
  }

  std::ifstream csv_file(csv_filename);
  if(!csv_file.is_open()) {
    std::cerr << "Error opening file " << csv_filename << std::endl;
    exit(-1);
  }

  // Extract header from CSV file
  std::string csv_header;
  std::getline(csv_file, csv_header);

  std::cout << "CSV header: \"" << csv_header << "\"" << std::endl;

  std::vector<std::string> csv_fields;

  {
    size_t current_position = 0;
    size_t next_position;
    size_t len;

    while(current_position != std::string::npos) {
      next_position = csv_header.find(csv_delim, current_position);
      if(next_position != std::string::npos) {
        len = next_position - current_position;
        next_position += 1;
      } else {
        len = std::string::npos;
      }

      std::string field_name = csv_header.substr(current_position, len);
      csv_fields.push_back(field_name);
      std::cout << field_name << std::endl;
      current_position = next_position;
    }
  }

  TH1I* h0 = new TH1I("h0", "#Deltat", 100, 0, 0);
  std::vector<TH1I*> h_vector;
  for(unsigned int i = 1; i < csv_fields.size(); i++) {
    std::string h_name = std::string("h") + std::to_string(i+1);
    h_vector.push_back(new TH1I(h_name.c_str(), csv_fields[i].c_str(), 1000, 0, 0));
    std::cout << "Created histogram " << h_name << " : " << csv_fields[i] << std::endl;
  }

  long value;

  csv_file.setf(std::ios::skipws);
  while(csv_file.good()) {
    std::string csv_line;
    std::getline(csv_file, csv_line);
    //std::cout << "csv_line: " << csv_line << std::endl;

    size_t current_position = 0;
    size_t next_position;
    size_t len;
    int i = 0;

    while(current_position != std::string::npos && csv_line.length() > 0) {
      next_position = csv_line.find(csv_delim, current_position);
      if(next_position != std::string::npos) {
        len = next_position - current_position;
        next_position += 1;
      } else {
        len = std::string::npos;
      }

      std::string value_str = csv_line.substr(current_position, len);
      //std::cout << "value_str: " << value_str << std::endl;
      int value = std::stoi(value_str);
      //std::cout << "value: " << value << std::endl;

      // First column has time between events
      if(i == 0) {
        h0->Fill(value);
        event_time_vec.push_back(value);
      }
      else { // The next column has multiplicities
        h_vector[i-1]->Fill(value);
      }
      i++;

      current_position = next_position;
    }
  }


  TCanvas* c1 = new TCanvas();
  h0->Draw();
  h0->Write();

  if(create_png)
    c1->Print("png/event_rate.png", "png");
  if(create_pdf)
    c1->Print("pdf/event_rate.pdf", "pdf");

  summary_file << "Mean delta t: " << h0->GetMean() << " ns" << std::endl;
  summary_file << "Average event rate: " << (int(1.0E9) / h0->GetMean()) / 1000 << " kHz" << std::endl;



  TCanvas* c2 = new TCanvas();
  for(auto it = h_vector.begin(); it != h_vector.end(); it++) {
    (*it)->Draw();
    (*it)->Write();
    std::string plot_title = (*it)->GetTitle();
    std::string plot_file_linear_png = sim_run_data_path + "png/" + plot_title + std::string("-linear.png");
    std::string plot_file_linear_pdf = sim_run_data_path + "pdf/" + plot_title + std::string("-linear.pdf");
    std::string plot_file_log_png = sim_run_data_path + "png/" + plot_title + std::string("-log.png");
    std::string plot_file_log_pdf = sim_run_data_path + "pdf/" + plot_title + std::string("-log.pdf");

    c2->SetLogy(0);
    if(create_png)
      c2->Print(plot_file_linear_png.c_str(), "png");
    if(create_pdf)
      c2->Print(plot_file_linear_pdf.c_str(), "pdf");

    c2->SetLogy(1);
    if(create_png)
      c2->Print(plot_file_log_png.c_str(), "png");
    if(create_pdf)
      c2->Print(plot_file_log_pdf.c_str(), "pdf");

    summary_file << std::endl;
    summary_file << plot_title << ": " << std::endl;

    if(plot_title.find("multiplicity") != std::string::npos) {
      int num_chips = (h_vector.size()-1)/2;
      double total_area = chip_width_cm*chip_height_cm*num_chips;
      summary_file << "\tAverage number of hits: " << (*it)->GetMean() << std::endl;
      summary_file << "\tHit density: " << (*it)->GetMean()/total_area << " hits/cm^2" << std::endl;
    }
    else if(plot_title.find("pixel") != std::string::npos) {
      summary_file << "\tAverage number of pixel hits: " << (*it)->GetMean() << std::endl;
      summary_file << "\tHit density: " << (*it)->GetMean()/(chip_width_cm*chip_height_cm) << " pixel hits/cm^2" << std::endl;
    }
    else if(plot_title.find("trace") != std::string::npos) {
      summary_file << "\tAverage number of trace hits: " << (*it)->GetMean() << std::endl;
      summary_file << "\tHit density: " << (*it)->GetMean()/(chip_width_cm*chip_height_cm) << " trace hits/cm^2" << std::endl;
    }
  }

  // Cleanup
  delete h0;
  for(auto it = h_vector.begin(); it != h_vector.end(); it++)
    delete *it;

  delete c1;
  delete c2;
  delete f;

  return event_time_vec;
}


void print_help(void)
{
  std::cout << std::endl;
  std::cout << "Usage:" << std::endl;
  std::cout << "process_readout_trigger_stats [optional arguments] <path_to_sim_data>" << std::endl;
  std::cout << std::endl;
  std::cout << "Optional arguments: " << std::endl;
  std::cout << "-h, --help: \tPrint this screen" << std::endl;
  std::cout << "-png, --png: \tWrite all plots to PNG files." << std::endl;
  std::cout << "-pdf, --pdf: \tWrite all plots to PDF files." << std::endl;
}


# ifndef __CINT__
int main(int argc, char** argv)
{
  std::string png_path = "";
  std::string pdf_path = "";

  bool create_png = false;
  bool create_pdf = false;

  if(argc == 1) {
    print_help();
    exit(0);
  }

  for(unsigned int arg_num = 1; arg_num < argc; arg_num++) {
    if(strcmp(argv[arg_num], "-png") == 0 || strcmp(argv[arg_num], "--png") == 0) {
      png_path = std::string(argv[argc-1]) + "/png";
      std::cout << "Creating directory " << pdf_path << std::endl;
      std::string mkdir_png_path = "mkdir " + png_path;
      system(mkdir_png_path.c_str());
      create_png = true;
    }
    else if(strcmp(argv[arg_num], "-pdf") == 0 || strcmp(argv[arg_num], "--pdf") == 0) {
      pdf_path = std::string(argv[argc-1]) + "/pdf";
      std::cout << "Creating directory " << pdf_path << std::endl;
      std::string mkdir_pdf_path = "mkdir " + pdf_path;
      system(mkdir_pdf_path.c_str());
      create_pdf = true;
    }
    else if(strcmp(argv[arg_num], "-h") == 0 || strcmp(argv[arg_num], "--help") == 0) {
      print_help();
      exit(0);
    }
    else if(arg_num < argc-1) {
      // If "last argument" is unknown then ignore that one,
      // because it has to be the file name
      std::cout << "Unknown argument " << argv[arg_num] << std::endl;
      print_help();
      exit(0);
    }
  }

  process_readout_trigger_stats(argv[argc-1], create_png, create_pdf);
  return 0;
}
# endif
