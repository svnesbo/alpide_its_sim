#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <vector>
#include <TFile.h>
#include <TCanvas.h>
#include <TH1F.h>
#include <TH1I.h>


const std::string delim(";");

const float chip_width_cm = 3.0;
const float chip_height_cm = 1.5;

int process_event_data(const char* csv_filename)
{
  std::string csv_filename_str = std::string(csv_filename);
  size_t csv_extension_start = csv_filename_str.find(".csv");
  std::string root_filename;

  system("mkdir png");
  system("mkdir pdf");

  // If we didn't find any /, then the CSV file must reside in the current directory
  if(csv_extension_start == std::string::npos) {
    std::cerr << "Error. Expected .csv file." << std::endl;
    exit(-1);
  } else {
    root_filename = csv_filename_str.substr(0, csv_extension_start) + std::string(".root");
    std::cout << "Root filename: " << root_filename << std::endl;
  }

  TFile *f = new TFile(root_filename.c_str(), "recreate");

  size_t csv_file_base_path_start = csv_filename_str.rfind("/");
  std::string summary_filename;
  
  if(csv_file_base_path_start == std::string::npos) {
    // No / found? Current directory must be the data output directory then
    summary_filename = "summary.txt";
  } else {
    summary_filename = csv_filename_str.substr(0, csv_file_base_path_start) + "/summary.txt";
  }

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
      next_position = csv_header.find(delim, current_position);
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
      next_position = csv_line.find(delim, current_position);
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

      if(i == 0) {
        h0->Fill(value);
      } else {
        h_vector[i-1]->Fill(value);
      }
      i++;

      current_position = next_position;      
    }    
  }


  TCanvas* c1 = new TCanvas();
  h0->Draw();
  h0->Write();
  
  c1->Print("png/event_rate.png", "png");
  c1->Print("pdf/event_rate.pdf", "pdf");
  
  summary_file << "Mean delta t: " << h0->GetMean() << " ns" << std::endl;
  summary_file << "Average event rate: " << (int(1.0E9) / h0->GetMean()) / 1000 << " kHz" << std::endl;

  

  TCanvas* c2 = new TCanvas();
  for(auto it = h_vector.begin(); it != h_vector.end(); it++) {
    (*it)->Draw();
    (*it)->Write();
    std::string plot_title = (*it)->GetTitle();
    std::string plot_file_linear_png = std::string("png/") + plot_title + std::string("-linear.png");
    std::string plot_file_linear_pdf = std::string("pdf/") + plot_title + std::string("-linear.pdf");
    std::string plot_file_log_png = std::string("png/") + plot_title + std::string("-log.png");
    std::string plot_file_log_pdf = std::string("pdf/") + plot_title + std::string("-log.pdf");    

    c2->SetLogy(0);
    c2->Print(plot_file_linear_png.c_str(), "png");
    c2->Print(plot_file_linear_pdf.c_str(), "pdf");
    c2->SetLogy(1);
    c2->Print(plot_file_log_png.c_str(), "png");
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
  

  return 0;

  
  // Cleanup - makes the macro crash (lol?)
  delete h0;
  for(auto it = h_vector.begin(); it != h_vector.end(); it++)
    delete *it;

  delete c1;
  delete c2;
}
