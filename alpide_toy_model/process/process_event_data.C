#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <TFile.h>
#include <TCanvas.h>
#include <TH1F.h>
#include <TH1I.h>

const std::string delim(";");

void process_event_data(const char* csv_filename)
{
  std::ifstream csv_file(csv_filename);

  if(!csv_file.is_open()) {
    std::cout << "Error opening file " << csv_filename << std::endl;
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

  TH1I* h0 = new TH1I("h0", "delta_t", 100, 0, 0);
  std::vector<TH1I*> h_vector;
  for(unsigned int i = 1; i < csv_fields.size(); i++) {
    std::string h_name = std::string("h") + std::to_string(i+1);
    h_vector.push_back(new TH1I(h_name.c_str(), csv_fields[i].c_str(), 1000, 0, 0));
    std::cout << "Created histogram " << h_name << " : " << csv_fields[i] << std::endl;
  }

  long value;
  
  csv_file.setf(std::ios::skipws);  
  while(csv_file.good()) {
    // for(unsigned int i = 0; i < csv_fields.size(); i++) {
    //   if(i == 0) {
    //     csv_file >> value;
    //     h0->Fill(value);
    //   } else {
    //     char c;
    //     csv_file >> c;  // Get delimiter
    //     csv_file >> value;
    //     h_vector[i-1]->Fill(value);
    //   }
    //   std::cout << "Column " << i << ", read value: " << value << std::endl;
    // }
    std::string csv_line;
    std::getline(csv_file, csv_line);
    std::cout << "csv_line: " << csv_line << std::endl;
    
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
//      value_str = value_str.find_first_not_of(" \t\n"); // Strip whitespace
      std::cout << "value_str: " << value_str << std::endl;
      int value = std::stoi(value_str);
      std::cout << "value: " << value << std::endl;

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

  TCanvas* c2 = new TCanvas();
  for(auto it = h_vector.begin(); it != h_vector.end(); it++)
    (*it)->Draw();

  // Cleanup
  // delete h0;
  // for(auto it = h_vector.begin(); it != h_vector.end(); it++)
  //   delete *it;

  // delete c1;
  // delete c2;

//  for(auto it = csv_fields.begin(); it != csv_fields.end(); it++)
//    std::cout << *it << std::endl;

}
