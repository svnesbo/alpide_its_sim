#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <vector>
#include <TFile.h>
#include <TCanvas.h>
#include <TApplication.h>
#include <TH1F.h>
#include <TH1I.h>
#include "ReadoutUnitStats.hpp"
#include "ITSLayerStats.hpp"

//enum TrigAction {TRIGGER_SENT, TRIGGER_NOT_SENT_BUSY, TRIGGER_FILTERED};

int process_readout_trigger_stats(const char* sim_run_data_path)
{
  // Create application environment, needed for interactive apps.
  // Alternatively can create a TRint object, I which case you
  // get also command line input capability.
  TApplication theApp("App", 0, 0);

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
  ITSLayerStats ITS_layer(0, 12, sim_run_data_path);



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



# ifndef __CINT__
int main(int argc, char** argv)
{
  process_readout_trigger_stats(argv[1]);
  return 0;
}
# endif
