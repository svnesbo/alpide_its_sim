/**
 * @file   ITSSimulationStats.cpp
 * @author Simon Voigt Nesbo
 * @date   August 28, 2017
 * @brief  Functions etc. for writing simulation stats to file for ITS/ALPIDE
 */

#include "ITSSimulationStats.hpp"
#include "ITS_config.hpp"


using namespace ITS;

///@brief Write simulation data to file. Histograms for MEB usage from the Alpide chips,
///       and event frame statistics (number of accepted/rejected) in the chips are recorded here
///@param[in] alpide_vec Vector of pointers to Alpide chip objects. Vector can contain
///           null pointers, which will be ignored.
void writeAlpideStatsToFile(std::string output_path,
                            const std::vector<std::shared_ptr<Alpide>>& alpide_vec)
{
  std::vector<std::map<unsigned int, std::uint64_t> > alpide_histos;
  unsigned int all_histos_biggest_key = 0;

  std::string csv_filename = output_path + std::string("/Alpide_MEB_histograms.csv");
  ofstream csv_file(csv_filename);

  if(!csv_file.is_open()) {
    std::cerr << "Error opening CSV file for histograms: " << csv_filename << std::endl;
    return;
  }

  csv_file << "Multi Event Buffers in use";

  std::cout << "Getting MEB histograms from chips. " << std::endl;

  // Get histograms from chip objects, and finish writing CSV header
  for(auto it = alpide_vec.begin(); it != alpide_vec.end(); it++) {
    if(*it != nullptr) {
      int chip_id = (*it)->getChipId();
      csv_file << ";Chip ID " << chip_id;

      alpide_histos.push_back((*it)->getMEBHisto());

      // Check and possibly update the biggest MEB size (key) found in the histograms
      auto current_histo = alpide_histos.back();
      if(current_histo.rbegin() != current_histo.rend()) {
        unsigned int current_histo_biggest_key = current_histo.rbegin()->first;
        if(all_histos_biggest_key < current_histo_biggest_key)
          all_histos_biggest_key = current_histo_biggest_key;
      }
    }
  }


  std::cout << "Writing MEB histograms to file. " << std::endl;


  // Write values to CSV file
  for(unsigned int MEB_size = 0; MEB_size <= all_histos_biggest_key; MEB_size++) {
    csv_file << std::endl;
    csv_file << MEB_size;

    for(unsigned int i = 0; i < alpide_histos.size(); i++) {
      csv_file << ";";

      auto histo_it = alpide_histos[i].find(MEB_size);

      // Write value if it was found in histogram
      if(histo_it != alpide_histos[i].end())
        csv_file << histo_it->second;
      else
        csv_file << 0;
    }
  }


  std::cout << "Writing Alpide stats to file. " << std::endl;

  std::string trigger_stats_filename = output_path + std::string("/Alpide_stats.csv");
  ofstream trigger_stats_file(trigger_stats_filename);

  trigger_stats_file << "Layer ID; Stave ID; Sub-stave ID; Module ID; Local Chip ID; Unique Chip ID; ";
  trigger_stats_file << "Accepted triggers; Rejected triggers; ";
  trigger_stats_file << "Busy; Busy violations; Flushed Incompletes" << std::endl;

  for(auto it = alpide_vec.begin(); it != alpide_vec.end(); it++) {
    if(*it != nullptr) {
      unsigned int unique_chip_id = (*it)->getChipId();
      detectorPosition pos = chip_id_to_detector_position(unique_chip_id);

      trigger_stats_file << pos.layer_id << ";";
      trigger_stats_file << pos.stave_id << ";";
      trigger_stats_file << pos.sub_stave_id << ";";
      trigger_stats_file << pos.module_id << ";";
      trigger_stats_file << pos.module_chip_id << ";";
      trigger_stats_file << unique_chip_id << ";";
      trigger_stats_file << (*it)->getTriggersAcceptedCount() << ";";
      trigger_stats_file << (*it)->getTriggersRejectedCount() << ";";
      trigger_stats_file << (*it)->getBusyCount() << ";";
      trigger_stats_file << (*it)->getBusyViolationCount() << ";";
      trigger_stats_file << (*it)->getFlushedIncompleteCount() << ";" << std::endl;
    }
  }

}
