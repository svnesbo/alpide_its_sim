/**
 * @file   Settings.cpp
 * @author Simon Voigt Nesbo <svn@hib.no>
 * @date   November 3, 2016
 * @brief  Source file for simulation settings file.
 *
 *         Some functions for reading the simulation settings file, and for initializing
 *         default settings if the settings file, or certain settings, are missing.
 */

#include "Settings.hpp"
#include <QStringList>



///@brief Open a file with simulation settings. Should reside in config/ directory, relative
///       to the current working directory.
///       If the file does not exist, it will be created. If any settings are missing,
///       they will be initialized with default values.
///       If no filename is specified, the default settings.txt file is used in the
///       current directory.
///@param[in] fileName File to open, relative to current directory.
///           Defaults to settings.txt if not supplied.
///@return Pointer to QSettings object initialized with all settings, either from settings file or
///        with default settings if any settings were missing.
QSettings *getSimSettings(const char *fileName) {
  QString fileNameFullPath = QDir::currentPath() + "/config/" + fileName;
  QSettings *readoutSimSettings = new QSettings(fileNameFullPath, QSettings::IniFormat);

  // Sync QSettings object with file settings file contents
  readoutSimSettings->sync();

  // Set default settings for settings that are not specified in the settings file.
  setDefaultSimSettings(readoutSimSettings);

  // Sync settings file with (potentially) updated settings in QSettings object
  readoutSimSettings->sync();

  return readoutSimSettings;
}


///@brief Set default settings for each setting that is missing in the QSettings object.
///@param[in,out] readoutSimSettings Pointer to QSettings object.
void setDefaultSimSettings(QSettings *readoutSimSettings) {
  QMap<QString, QString> defaultSettings;

  // Default settings map
  defaultSettings["data_output/write_vcd"] = DEFAULT_DATA_OUTPUT_WRITE_VCD;
  defaultSettings["data_output/write_vcd_clock"] = DEFAULT_DATA_OUTPUT_WRITE_VCD_CLOCK;
  defaultSettings["data_output/write_event_csv"] = DEFAULT_DATA_OUTPUT_WRITE_EVENT_CSV;

  defaultSettings["simulation/type"] = DEFAULT_SIMULATION_TYPE;
  defaultSettings["simulation/single_chip"] = DEFAULT_SIMULATION_SINGLE_CHIP;
  defaultSettings["simulation/n_events"] = DEFAULT_SIMULATION_N_EVENTS;
  defaultSettings["simulation/continuous_mode"] = DEFAULT_SIMULATION_CONTINUOUS_MODE;
  defaultSettings["simulation/random_seed"] = DEFAULT_SIMULATION_RANDOM_SEED;

  defaultSettings["alpide/data_long_enable"] = DEFAULT_ALPIDE_DATA_LONG_ENABLE;
  defaultSettings["alpide/dtu_delay"] = DEFAULT_ALPIDE_DTU_DELAY;
  defaultSettings["alpide/pixel_shaping_dead_time_ns"] = DEFAULT_ALPIDE_PIXEL_SHAPING_DEAD_TIME_NS;
  defaultSettings["alpide/pixel_shaping_active_time_ns"] = DEFAULT_ALPIDE_PIXEL_SHAPING_ACTIVE_TIME_NS;
  defaultSettings["alpide/matrix_readout_speed_fast"] = DEFAULT_ALPIDE_MATRIX_READOUT_SPEED_FAST;
  defaultSettings["alpide/strobe_extension_enable"] = DEFAULT_ALPIDE_STROBE_EXTENSION_ENABLE;
  defaultSettings["alpide/minimum_busy_cycles"] = DEFAULT_ALPIDE_MINIMUM_BUSY_CYCLES;

  defaultSettings["its/layer0_num_staves"] = DEFAULT_ITS_LAYER0_NUM_STAVES;
  defaultSettings["its/layer1_num_staves"] = DEFAULT_ITS_LAYER1_NUM_STAVES;
  defaultSettings["its/layer2_num_staves"] = DEFAULT_ITS_LAYER2_NUM_STAVES;
  defaultSettings["its/layer3_num_staves"] = DEFAULT_ITS_LAYER3_NUM_STAVES;
  defaultSettings["its/layer4_num_staves"] = DEFAULT_ITS_LAYER4_NUM_STAVES;
  defaultSettings["its/layer5_num_staves"] = DEFAULT_ITS_LAYER5_NUM_STAVES;
  defaultSettings["its/layer6_num_staves"] = DEFAULT_ITS_LAYER6_NUM_STAVES;

  defaultSettings["pct/num_staves"] = DEFAULT_PCT_NUM_STAVES;

  defaultSettings["event/random_hit_generation"] = DEFAULT_EVENT_RANDOM_HIT_GENERATION;

  defaultSettings["event/monte_carlo_file_type"] = DEFAULT_EVENT_MONTE_CARLO_FILE_TYPE;
  defaultSettings["event/qed_noise_path"] = DEFAULT_EVENT_QED_NOISE_PATH;
  defaultSettings["event/qed_noise_input"] = DEFAULT_EVENT_QED_NOISE_INPUT;
  defaultSettings["event/qed_noise_feed_rate_ns"] = DEFAULT_EVENT_QED_NOISE_FEED_RATE_NS;
  defaultSettings["event/qed_noise_event_rate_ns"] = DEFAULT_EVENT_QED_NOISE_EVENT_RATE_NS;
  defaultSettings["event/trigger_delay_ns"] = DEFAULT_EVENT_TRIGGER_DELAY_NS;
  defaultSettings["event/trigger_filter_time_ns"] = DEFAULT_EVENT_TRIGGER_FILTER_TIME_NS;
  defaultSettings["event/trigger_filter_enable"] = DEFAULT_EVENT_TRIGGER_FILTER_ENABLE;
  defaultSettings["event/strobe_active_length_ns"] = DEFAULT_EVENT_STROBE_ACTIVE_LENGTH_NS;
  defaultSettings["event/strobe_inactive_length_ns"] = DEFAULT_EVENT_STROBE_INACTIVE_LENGTH_NS;
  ///@todo Rename to average_trigger_rate_ns?
  defaultSettings["event/average_event_rate_ns"] = DEFAULT_EVENT_AVERAGE_EVENT_RATE_NS;

  defaultSettings["event/its/hit_multiplicity_distribution_file"] = DEFAULT_EVENT_ITS_HIT_MULTIPLICITY_DISTRIBUTION_FILE;
  defaultSettings["event/its/bunch_crossing_rate_ns"] = DEFAULT_EVENT_ITS_BUNCH_CROSSING_RATE_NS;
  defaultSettings["event/its/monte_carlo_dir_path"] = DEFAULT_EVENT_ITS_MONTE_CARLO_DIR_PATH;
  defaultSettings["event/its/hit_density_layer0"] = DEFAULT_EVENT_ITS_HIT_DENSITY_LAYER0;
  defaultSettings["event/its/hit_density_layer1"] = DEFAULT_EVENT_ITS_HIT_DENSITY_LAYER1;
  defaultSettings["event/its/hit_density_layer2"] = DEFAULT_EVENT_ITS_HIT_DENSITY_LAYER2;
  defaultSettings["event/its/hit_density_layer3"] = DEFAULT_EVENT_ITS_HIT_DENSITY_LAYER3;
  defaultSettings["event/its/hit_density_layer4"] = DEFAULT_EVENT_ITS_HIT_DENSITY_LAYER4;
  defaultSettings["event/its/hit_density_layer5"] = DEFAULT_EVENT_ITS_HIT_DENSITY_LAYER5;
  defaultSettings["event/its/hit_density_layer6"] = DEFAULT_EVENT_ITS_HIT_DENSITY_LAYER6;

  defaultSettings["event/pct/monte_carlo_file_path"] = DEFAULT_EVENT_PCT_MONTE_CARLO_FILE_PATH;
  defaultSettings["event/pct/time_frame_length_ns"] = DEFAULT_EVENT_PCT_TIME_FRAME_LENGTH_NS;
  defaultSettings["event/pct/random_flux_mean_per_s"] = DEFAULT_EVENT_PCT_RANDOM_FLUX_MEAN_PER_S;
  defaultSettings["event/pct/random_flux_stddev_per_s"] = DEFAULT_EVENT_PCT_RANDOM_FLUX_STDDEV_PER_S;
  defaultSettings["event/pct/random_beam_diameter_mean_mm"] = DEFAULT_EVENT_PCT_RANDOM_BEAM_DIAMETER_MEAN_MM;
  defaultSettings["event/pct/random_beam_diameter_stddev_mm"] = DEFAULT_EVENT_PCT_RANDOM_BEAM_DIAMETER_STDDEV_MM;


  QStringList simSettingsKeys = readoutSimSettings->allKeys();

  QMap<QString, QString>::iterator i = defaultSettings.begin();

  while(i != defaultSettings.end()) {
    // Initialize each key missing in readoutSimSettings with a default value
    if(simSettingsKeys.contains(i.key()) == false) {
      //qInfo() << "Key is missing. Updating value" << endl;
      readoutSimSettings->setValue(i.key(), defaultSettings[i.key()]);
    }

    i++;
  }

  return;
}