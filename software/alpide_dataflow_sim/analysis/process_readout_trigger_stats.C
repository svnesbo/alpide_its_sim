#include <iostream>
#include <memory>
#include <iomanip>
#include <fstream>
#include <cstring>
#include <string>
#include <vector>
#include <cmath>
#include <TFile.h>
#include <TCanvas.h>
#include <TROOT.h>
//#include <TApplication.h>
#include <TH1F.h>
#include <TH1I.h>
#include "DetectorStats.hpp"
#include "EventData.hpp"
#include "../src/Settings/Settings.hpp"
#include "../src/Detector/ITS/ITSDetectorConfig.hpp"
#include "../src/Detector/PCT/PCTDetectorConfig.hpp"

const std::string csv_delim(";");

const float chip_width_cm = 3.0;
const float chip_height_cm = 1.5;

unsigned long get_num_triggered_events_simulated(std::string sim_run_data_path);
unsigned long get_num_untriggered_events_simulated(std::string sim_run_data_path);
EventData process_event_data(std::string sim_run_data_path,
                             std::string filename_csv,
                             bool create_png, bool create_pdf);



int process_its_readout_trigger_stats(const char* sim_run_data_path,
                                      bool create_png,
                                      bool create_pdf,
                                      const QSettings* sim_settings)
{
  unsigned int event_rate_ns = sim_settings->value("event/average_event_rate_ns").toUInt();

  // Round event rate to nearest kilohertz
  double event_rate_khz = std::round(1.0E6 / event_rate_ns);

  std::cout << "Event rate: " << event_rate_khz << " kHz" << std::endl;

  bool single_chip_mode = sim_settings->value("simulation/single_chip").toBool();

  ITS::ITSDetectorConfig det_config;

  det_config.layer[0].num_staves = sim_settings->value("its/layer0_num_staves").toInt();
  det_config.layer[1].num_staves = sim_settings->value("its/layer1_num_staves").toInt();
  det_config.layer[2].num_staves = sim_settings->value("its/layer2_num_staves").toInt();
  det_config.layer[3].num_staves = sim_settings->value("its/layer3_num_staves").toInt();
  det_config.layer[4].num_staves = sim_settings->value("its/layer4_num_staves").toInt();
  det_config.layer[5].num_staves = sim_settings->value("its/layer5_num_staves").toInt();
  det_config.layer[6].num_staves = sim_settings->value("its/layer6_num_staves").toInt();

  bool event_csv_available = sim_settings->value("data_output/write_event_csv").toBool();

  std::cout << "Single chip mode: " << (single_chip_mode ? "true" : "false") << std::endl;
  std::cout << "Staves layer 0: " << det_config.layer[0].num_staves << std::endl;
  std::cout << "Staves layer 1: " << det_config.layer[1].num_staves << std::endl;
  std::cout << "Staves layer 2: " << det_config.layer[2].num_staves << std::endl;
  std::cout << "Staves layer 3: " << det_config.layer[3].num_staves << std::endl;
  std::cout << "Staves layer 4: " << det_config.layer[4].num_staves << std::endl;
  std::cout << "Staves layer 5: " << det_config.layer[5].num_staves << std::endl;
  std::cout << "Staves layer 6: " << det_config.layer[6].num_staves << std::endl;
  std::cout << "Event CSV file available: " << (event_csv_available ? "true" : "false") << std::endl;

  gROOT->SetBatch(kTRUE);

  std::shared_ptr<EventData> event_data;

  if(event_csv_available) {
    event_data = std::make_shared<EventData>(process_event_data(sim_run_data_path,
                                                                "physics_events_data.csv",
                                                                create_png, create_pdf));
  }

  unsigned long num_physics_events = get_num_triggered_events_simulated(sim_run_data_path);
  unsigned long sim_time_ns = event_rate_ns*num_physics_events;

  std::cout << "Num physics_events: " << num_physics_events << std::endl;
  std::cout << "Event rate (ns): " << event_rate_ns << std::endl;
  std::cout << "Sim time (ns): " << sim_time_ns << std::endl;

  std::map<std::string, double> sim_params;
  sim_params["event_rate_khz"] = event_rate_khz;

  DetectorStats its_detector_stats(det_config, sim_params,
                                   sim_time_ns, "its",
                                   sim_run_data_path,
                                   event_data);

  its_detector_stats.plotDetector(create_png, create_pdf);

  return 0;
}


int process_pct_readout_trigger_stats(const char* sim_run_data_path,
                                      bool create_png,
                                      bool create_pdf,
                                      const QSettings* sim_settings,
                                      QString sim_type)
{
  PCT::PCTDetectorConfig det_config;

  unsigned long time_frame_length_ns = sim_settings->value("pct/time_frame_length_ns").toInt();

  det_config.num_layers = sim_settings->value("pct/num_layers").toInt();

  std::cout << "Number of layers " << det_config.num_layers << std::endl;

  for(unsigned int lay_num = 0; lay_num < det_config.layer.size(); lay_num++) {
    if(lay_num < det_config.num_layers)
      det_config.layer[lay_num].num_staves = sim_settings->value("pct/num_staves_per_layer").toInt();
    else
      det_config.layer[lay_num].num_staves = 0;

    std::cout << "Staves layer " << lay_num << ": " << det_config.layer[lay_num].num_staves << std::endl;
  }

  bool single_chip_mode = sim_settings->value("simulation/single_chip").toBool();
  std::cout << "Single chip mode: " << (single_chip_mode ? "true" : "false") << std::endl;

  bool event_csv_available = sim_settings->value("data_output/write_event_csv").toBool();
  std::cout << "Event CSV file available: " << (event_csv_available ? "true" : "false") << std::endl;

  gROOT->SetBatch(kTRUE);

  std::shared_ptr<EventData> event_data;

  unsigned long num_event_frames;

  if(sim_type == "pct") {
    num_event_frames = get_num_untriggered_events_simulated(sim_run_data_path);
  } else {
    // Focal
    num_event_frames = get_num_triggered_events_simulated(sim_run_data_path);

    if(event_csv_available) {
      event_data = std::make_shared<EventData>(process_event_data(sim_run_data_path,
                                                                  "physics_events_data.csv",
                                                                  create_png, create_pdf));
    }
  }

  unsigned long sim_time_ns = time_frame_length_ns*num_event_frames;

  std::map<std::string, double> sim_params;
  sim_params["random_particles_per_s"] = sim_settings->value("pct/random_particles_per_s").toDouble();

  DetectorStats pct_detector_stats(det_config, sim_params,
                                   sim_time_ns, "pct",
                                   sim_run_data_path,
                                   event_data);

  pct_detector_stats.plotDetector(create_png, create_pdf);

  return 0;
}


int process_readout_trigger_stats(const char* sim_run_data_path,
                                  bool create_png,
                                  bool create_pdf)
{
  QString settings_file_path = QString(sim_run_data_path) + "/settings.txt";
  QSettings *sim_settings = new QSettings(settings_file_path, QSettings::IniFormat);

  QString sim_type = sim_settings->value("simulation/type").toString();

  if(sim_type == "its") {
    process_its_readout_trigger_stats(sim_run_data_path,
                                      create_png,
                                      create_pdf,
                                      sim_settings);
  } else if(sim_type == "pct" || sim_type == "focal"){
    process_pct_readout_trigger_stats(sim_run_data_path,
                                      create_png,
                                      create_pdf,
                                      sim_settings,
                                      sim_type);
  } else {
    std::cerr << "Unknown simulation type." << std::endl;
    exit(-1);
  }

}

///@brief Get the number of triggered events actually simulated.
///       Will exit if simulation_info.txt file can not be opened,
///       or if there is a problem reading the file.
unsigned long get_num_triggered_events_simulated(std::string sim_run_data_path)
{
  std::string sim_info_filename = sim_run_data_path + "/simulation_info.txt";

  std::ifstream sim_info_file(sim_info_filename);
  if(!sim_info_file.is_open()) {
    std::cerr << "Error opening file " << sim_info_filename << std::endl;
    exit(-1);
  }

  // Number of triggered events simulated should be on the second line
  std::string events_simulated_str;
  std::getline(sim_info_file, events_simulated_str);
  std::getline(sim_info_file, events_simulated_str);

  if(events_simulated_str.find("Number of triggered events simulated: ") == std::string::npos) {
    std::cout << "Error: number of triggered events simulated not found in ";
    std::cout << sim_info_filename << std::endl;
    exit(-1);
  }

  size_t text_len = strlen("Number of triggered events simulated: ");
  std::string num_events_str = events_simulated_str.substr(text_len);
  std::cout << "num_events_str: " << num_events_str << std::endl;
  unsigned long num_events = std::stoul(num_events_str);
  if(num_events == 0) {
    std::cout << "Error: no events simulated?" << std::endl;
    exit(-1);
  }

  return num_events;
}


///@brief Get the number of untriggered events actually simulated.
///       Will exit if simulation_info.txt file can not be opened,
///       or if there is a problem reading the file.
unsigned long get_num_untriggered_events_simulated(std::string sim_run_data_path)
{
  std::string sim_info_filename = sim_run_data_path + "/simulation_info.txt";

  std::ifstream sim_info_file(sim_info_filename);
  if(!sim_info_file.is_open()) {
    std::cerr << "Error opening file " << sim_info_filename << std::endl;
    exit(-1);
  }

  // Number of untriggered events simulated should be on the fourth line
  std::string events_simulated_str;
  std::getline(sim_info_file, events_simulated_str);
  std::getline(sim_info_file, events_simulated_str);
  std::getline(sim_info_file, events_simulated_str);
  std::getline(sim_info_file, events_simulated_str);

  if(events_simulated_str.find("Number of untriggered events simulated: ") == std::string::npos) {
    std::cout << "Error: number of untriggered events simulated not found in ";
    std::cout << sim_info_filename << std::endl;
    exit(-1);
  }

  size_t text_len = strlen("Number of untriggered events simulated: ");
  std::string num_events_str = events_simulated_str.substr(text_len);
  unsigned long num_events = std::stoul(num_events_str);
  if(num_events == 0) {
    std::cout << "Error: no events simulated?" << std::endl;
    exit(-1);
  }

  return num_events;
}


///@brief Process CSV file with event rate and multiplicity data, generate plots of the data
///       and store them in both png and pdf format, if desired.
///       The first column in the CSV file is expected to be time to the previous event,
///       and the other columns are expected to be multiplicity data. It can have any number
///       of columns after the first column, and a histogram plot of multiplicity will be
///       generated for each column (one can plot multiplicity for each chip for instance).
///@param sim_run_data_path Path to directory with simulation run data
///@param filename_csv Filename of csv file (including .csv extension)
///@param create_png Set to true to save histograms as png files
///@param create_pdf Set to true to save histograms as pdf files
///@return EventData object with info about multiplicity and time between events
///        Empty EventData object is returned if something went wrong.
EventData process_event_data(std::string sim_run_data_path,
                             std::string filename_csv,
                             bool create_png, bool create_pdf)
{
  EventData event_data;
  std::string csv_filename = sim_run_data_path + "/" + filename_csv;
  std::string filename_base = csv_filename.substr(0, csv_filename.find(".csv"));
  std::string root_filename =  filename_base + ".root";
  std::string summary_filename = filename_base + "_summary.txt";

  TFile *f = new TFile(root_filename.c_str(), "recreate");

  std::ofstream summary_file(summary_filename);
  if(!summary_file.is_open()) {
    std::cerr << "Error opening file " << summary_filename << std::endl;
    return event_data;
  }

  std::ifstream csv_file(csv_filename);
  if(!csv_file.is_open()) {
    std::cerr << "Error opening file " << csv_filename << std::endl;
    return event_data;
  }

  // Extract header from CSV file
  std::string csv_header;
  std::getline(csv_file, csv_header);

  std::cout << "CSV header: \"" << csv_header << "\"" << std::endl;

  // Parse/read header
  {
    size_t current_position = 0;
    size_t next_position;
    size_t len;

    while(current_position != std::string::npos) {
      // Find header names, put them _all_ in multipl_entry_names vector
      // All the fields before chip_ fields are removed later..
      // delta_t does not have a corresponding entry in multipl_data..
      next_position = csv_header.find(csv_delim, current_position);
      if(next_position != std::string::npos) {
        len = next_position - current_position;
        next_position += 1;
      } else {
        len = std::string::npos;
      }

      std::string field_name = csv_header.substr(current_position, len);
      event_data.multipl_entry_names.push_back(field_name);
      std::cout << field_name << std::endl;
      current_position = next_position;
    }
  }


  long value;
  event_data.multipl_data.resize(event_data.multipl_entry_names.size()-1);

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
        event_data.event_time_vec.push_back(value);
      }
      else { // The next column has multiplicities
        event_data.multipl_data[i-1].push_back(value);
      }
      i++;

      current_position = next_position;
    }
  }

  // Find maximum time between events,
  // and initialize histogram to that size.
  unsigned int max_time = 0;
  std::vector<uint64_t>::iterator max_time_it =
    std::max_element(event_data.event_time_vec.begin(), event_data.event_time_vec.end());

  if(max_time_it != event_data.event_time_vec.end())
    max_time = *max_time_it;

  TH1I* h0 = new TH1I("h0", "#Deltat", max_time+1, 0, max_time);
  for(auto time_it = event_data.event_time_vec.begin();
      time_it != event_data.event_time_vec.end();
      time_it++)
  {
    h0->Fill(*time_it);
  }


  // Create a vector of histograms with an entry
  // for each multiplicity field in the CSV file
  std::vector<TH1I*> h_vector;
  for(unsigned int i = 1; i < event_data.multipl_entry_names.size(); i++) {
    std::string h_name = std::string("h") + std::to_string(i);

    // Find maximum multiplicity value,
    // and initialize histogram to that size
    unsigned int max_val = 0;
    std::vector<unsigned int>::iterator max_val_it =
      std::max_element(event_data.multipl_data[i-1].begin(), event_data.multipl_data[i-1].end());

    // Just draw an empty plot if vector was empty
    if(max_val_it != event_data.multipl_data[i-1].end())
      max_val = *max_val_it;

    h_vector.push_back(new TH1I(h_name.c_str(), event_data.multipl_entry_names[i].c_str(), max_val+1, 0, max_val));
    std::cout << "Created histogram " << h_name << " : " << event_data.multipl_entry_names[i] << std::endl;

    for(auto multipl_it = event_data.multipl_data[i-1].begin();
        multipl_it != event_data.multipl_data[i-1].end();
        multipl_it++)
    {
      h_vector.back()->Fill(*multipl_it);
    }
  }


  TCanvas* c1 = new TCanvas();
  h0->Draw();
  h0->Write();

  if(create_png)
    c1->Print(Form("%s/png/event_rate.png", sim_run_data_path.c_str()), "png");
  if(create_pdf)
    c1->Print(Form("%s/pdf/event_rate.pdf", sim_run_data_path.c_str()), "pdf");

  summary_file << "Mean delta t: " << h0->GetMean() << " ns" << std::endl;
  summary_file << "Average event rate: " << (int(1.0E9) / h0->GetMean()) / 1000.0 << " kHz" << std::endl;



  TCanvas* c2 = new TCanvas();
  for(auto it = h_vector.begin(); it != h_vector.end(); it++) {
    (*it)->Draw();
    (*it)->Write();
    std::string plot_title = (*it)->GetTitle();

    c2->SetLogy(0);
    if(create_png)
      c2->Print(Form("%s/png/%s-linear.png", sim_run_data_path.c_str(), plot_title.c_str()), "png");
    if(create_pdf)
      c2->Print(Form("%s/pdf/%s-linear.pdf", sim_run_data_path.c_str(), plot_title.c_str()), "pdf");

    c2->SetLogy(1);
    if(create_png)
      c2->Print(Form("%s/png/%s-log.png", sim_run_data_path.c_str(), plot_title.c_str()), "png");
    if(create_pdf)
      c2->Print(Form("%s/pdf/%s-log.pdf", sim_run_data_path.c_str(), plot_title.c_str()), "pdf");

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


  // Remove stuff that messes with our calculations later... :(

  // remove delta_t field entry (no corresponding entry in multipl_data)
  event_data.multipl_entry_names.erase(event_data.multipl_entry_names.begin());

  while(event_data.multipl_entry_names[0].find("chip") == std::string::npos) {
    // remove all fields and data not for chip columns in csv file
    event_data.multipl_entry_names.erase(event_data.multipl_entry_names.begin());
    event_data.multipl_data.erase(event_data.multipl_data.begin());
  }

  return event_data;
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
  std::cout << "-b, --brew: \tBrew coffee." << std::endl;
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
      std::cout << "Creating directory " << png_path << std::endl;
      std::string mkdir_png_path = "mkdir " + png_path;
      system(mkdir_png_path.c_str());

      mkdir_png_path += "/chip_event_plots";
      system(mkdir_png_path.c_str());

      create_png = true;
    }
    else if(strcmp(argv[arg_num], "-pdf") == 0 || strcmp(argv[arg_num], "--pdf") == 0) {
      pdf_path = std::string(argv[argc-1]) + "/pdf";
      std::cout << "Creating directory " << pdf_path << std::endl;
      std::string mkdir_pdf_path = "mkdir " + pdf_path;
      system(mkdir_pdf_path.c_str());

      mkdir_pdf_path += "/chip_event_plots";
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
