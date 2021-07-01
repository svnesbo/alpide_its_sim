#include "../src/Detector/ITS/ITSDetectorConfig.hpp"
#include "../src/Event/EventBinaryITS.hpp"
#include "../src/Event/EventXMLITS.hpp"

#include <TCanvas.h>
#include <TFile.h>
#include <TROOT.h>
#include <TTree.h>
#include <QDir>

//#include <cmath>
#include <cstring>
//#include <fstream>
#include <iostream>
//#include <iomanip>
#include <string>
#include <map>



//const std::string csv_delim(";");

// Required by PixelHit.hpp
std::int64_t g_num_pixels_in_mem = 0;

int get_mc_events_multiplicity(const char* mc_event_path, const char* mc_file_type, const char* output_filename)
{
  EventBaseDiscrete* events;
  const EventDigits* ev_digits;
  QStringList event_filenames;
  QStringList name_filters;
  QDir monte_carlo_event_dir(mc_event_path);
  ITS::ITSDetectorConfig cfg;

  if(std::string("xml") == mc_file_type)
  {
    name_filters << "*.xml";
    event_filenames = monte_carlo_event_dir.entryList(name_filters);

    if(event_filenames.isEmpty())
    {
      std::cerr << "Error: No .xml files found in MC event path";
      std::cerr << std::endl;
      exit(-1);
    }

    events = new EventXMLITS(cfg,
                             &ITS::ITS_global_chip_id_to_position,
                             &ITS::ITS_position_to_global_chip_id,
                             QString(mc_event_path),
                             event_filenames,
                             false);
  }
  else if(std::string("binary") == mc_file_type)
  {
    name_filters << "*.dat";
    event_filenames = monte_carlo_event_dir.entryList(name_filters);

    if(event_filenames.isEmpty())
    {
      std::cerr << "Error: No .dat files found in MC event path";
      std::cerr << std::endl;
      exit(-1);
    }

    events = new EventBinaryITS(cfg,
                                &ITS::ITS_global_chip_id_to_position,
                                &ITS::ITS_position_to_global_chip_id,
                                QString(mc_event_path),
                                event_filenames,
                                false);
  }
  else
  {
    std::cout << "Unknown MC file type \"" << mc_file_type << "\"." << std::endl;
    exit(-1);
  }

  TFile f(output_filename,"recreate");
  TTree* T = new TTree("event_multiplicity", "Event multiplicity");

  Int_t layers[7] = {0};

  for (int layer_num = 0; layer_num < 7; layer_num++)
  {
    T->Branch(Form("layer_%d", layer_num), &layers[layer_num]);
  }

  for(int ev_num = 0; ev_num < event_filenames.size(); ev_num++)
  {
    ev_digits = events->getNextEvent();

    // Key: layer number, value: multiplicity
    std::map<unsigned int, unsigned int> hit_multipl;

    for(auto dig_it = ev_digits->getDigitsIterator();
        dig_it != ev_digits->getDigitsEndIterator();
        dig_it++)
    {
      unsigned int layer = ITS::ITS_global_chip_id_to_position(dig_it->getChipId()).layer_id;
      hit_multipl[layer]++;
    }

    for(int lay_num = 0; lay_num < 7; lay_num++)
    {
      // Move values from the map to the array used by the TTree
      layers[lay_num] = hit_multipl[lay_num];
    }

    // Fill event multiplicities into TTree
    T->Fill();
  }

  T->Write();

  return 0;
}


void print_help(void)
{
  std::cout << std::endl;
  std::cout << "Create .root file with TTree of multiplicities for each layer" << std::endl;
  std::cout << "in MC event data for ITS in the SystemC simulations." << std::endl;
  std::cout << std::endl;
  std::cout << "Usage:" << std::endl;
  std::cout << "get_mc_events_multiplicity <path_to_mc_events> <binary/xml> <output_filename>" << std::endl;
}


# ifndef __CINT__
int main(int argc, char** argv)
{
  if(argc != 4) {
    print_help();
    exit(0);
  }

  return get_mc_events_multiplicity(argv[1], argv[2], argv[3]);
}
# endif
