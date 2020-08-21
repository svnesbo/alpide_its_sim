#include "get_num_events.hpp"

#include <cstring>
#include <iostream>
#include <fstream>


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
