#include "read_csv.hpp"

#include <iostream>
#include <memory>
#include <iomanip>
#include <fstream>
#include <cstring>
#include <cctype>
#include <cstdlib>


///@brief Split a string by delimiter
///@param str The string to split
///@param delim The delimiter character to split the string by
///@param skip_ws Skip whitespace (immediately following delimiter only)
///@return Vector with the strings, in the order they appeared
std::vector<std::string> split_string(const std::string& str, const char delim, bool skip_ws=true)
{
  std::vector<std::string> string_vec;

  size_t current_position = 0;
  size_t next_position;
  size_t len;

  while(current_position != std::string::npos) {
    next_position = str.find(delim, current_position);

    if(next_position != std::string::npos) {
      len = next_position - current_position;
      next_position += 1;
    } else {
      len = std::string::npos;
    }

    std::string sub_string = str.substr(current_position, len);

    string_vec.push_back(sub_string);

    current_position = next_position;

    // Skip white space following the delimiter?
    if(skip_ws && current_position != std::string::npos) {
      while(current_position < str.length() && std::isspace(str[current_position])) {
        current_position++;
      }
    }
  }

  return string_vec;
}


///@brief Read a csv file
///@param csv_file_path Full path to CSV file
///@param delim Delimiter character
///@param skip_ws Skip whitespace (immediately following delimiter only)
///@return Vector where each entry corresponds to a line in the csv, excluding header.
///        Each entry is a map where keys correspond to keys in the header, and values are read
///        from the line in the CSV file.
///        An empty vector is returned if reading the CSV failed.
std::vector<std::map<std::string, unsigned long> > read_csv(std::string csv_file_path,
                                                            const char delim, bool skip_ws)
{
  std::vector<std::map<std::string, unsigned long> > csv_data;

  std::ifstream csv_file(csv_file_path.c_str());
  if(!csv_file.is_open()) {
    std::cerr << "Error opening file " << csv_file_path << std::endl;
    return csv_data;
  }

  // Extract header from CSV file
  std::string csv_line;
  std::getline(csv_file, csv_line);

  std::cout << "CSV header: \"" << csv_line << "\"" << std::endl;

  // Parse/read header
  std::vector<std::string> csv_header = split_string(csv_line, delim, skip_ws);

  if(csv_header.empty())
    return csv_data;

  csv_file.setf(std::ios::skipws);

  unsigned int line_num = 2;

  // Read the CSV file line by line
  while(csv_file.good()) {
    std::getline(csv_file, csv_line);
    std::vector<std::string> csv_line_data = split_string(csv_line, delim);

    // If last line contains \n alone, then str.size() is 1
    if(csv_line_data.size() > 1 && csv_header.size() != csv_line_data.size()) {
      std::cout << "Error: Length of line " << line_num << " does not match header";
      std::cout << "(" << csv_line_data.size() << " vs " << csv_header.size();
      std::cout << ")" << std::endl;
      std::exit(-1);
    }

    if(csv_line_data.size() > 1) {
      std::map<std::string, unsigned long> csv_data_entry;

      for(unsigned int idx=0; idx < csv_header.size(); idx++) {
        csv_data_entry[csv_header[idx]] = std::atol(csv_line_data[idx].c_str());
      }

      csv_data.push_back(csv_data_entry);
    }

    line_num++;
  }

  csv_file.close();

  return csv_data;
}


#ifdef TEST
int main(void)
{
  std::vector<std::map<std::string, unsigned long> > dat;

  dat = read_csv("/scratch/ITS_SystemC/submodule_alpide_systemc/software/alpide_dataflow_sim/sim_output_focal_experiment/run_7/Alpide_stats.csv", ';');

  for(unsigned int idx = 0; idx < dat.size(); idx++) {
    std::cout << idx << ":";
    for(std::map<std::string, unsigned long>::const_iterator it = dat[idx].begin(); it != dat[idx].end(); it++) {
      std::cout << " '" << it->first << "': ";
      std::cout << it->second;
    }
    std::cout << std::endl;
  }

  return 0;
}
#endif
