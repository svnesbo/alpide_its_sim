/**
 * @file   settings.cpp
 * @Author Simon Voigt Nesbo <svn@hib.no>
 * @date   November 3, 2016
 * @brief  Some functions for reading the simulation settings file, and for initializing
 *         default settings if the settings file, or certain settings, are missing.
 *
 */

#include "settings.h"
#include <QStringList>



//@brief Open a file with simulation settings.
//       If the file does not exist, it will be created. If any settings are missing,
//       they will be initialized with default values.
//       If no filename is specified, the default settings.txt file is used in the
//       current directory.
//@param fileName File to open, relative to current directory. Defaults to settings.txt if not supplied.
//@return Pointer to QSettings object initialized with all settings, either from settings file or 
//        with default settings if any settings were missing.
QSettings *getSimSettings(const char *fileName) {
  QString fileNameFullPath = QDir::currentPath() + "/" + fileName;
  QSettings *readoutSimSettings = new QSettings(fileNameFullPath, QSettings::IniFormat);

  // Sync QSettings object with file settings file contents
  readoutSimSettings->sync();

  // Set default settings for settings that are not specified in the settings file.
  setDefaultSimSettings(readoutSimSettings);

  // Sync settings file with (potentially) updated settings in QSettings object
  readoutSimSettings->sync();

  return readoutSimSettings;
}


//@brief Set default settings for each setting that is missing in the QSettings object.
//@param readoutSimSettings Pointer to QSettings object.
void setDefaultSimSettings(QSettings *readoutSimSettings) {
  QMap<QString, QString> defaultSettings;

  // Default settings map
  defaultSettings["data_output/write_vcd"] = DEFAULT_DATA_OUTPUT_WRITE_VCD;
  defaultSettings["data_output/write_vcd_clock"] = DEFAULT_DATA_OUTPUT_WRITE_VCD_CLOCK;
  defaultSettings["data_output/write_event_csv"] = DEFAULT_DATA_OUTPUT_WRITE_EVENT_CSV;
  
  defaultSettings["simulation/n_chips"] = DEFAULT_SIMULATION_N_CHIPS;
  defaultSettings["simulation/n_events"] = DEFAULT_SIMULATION_N_EVENTS;
  defaultSettings["simulation/continuous_mode"] = DEFAULT_SIMULATION_CONTINUOUS_MODE;
  defaultSettings["simulation/random_seed"] = DEFAULT_SIMULATION_RANDOM_SEED;

  defaultSettings["event/hit_multiplicity_distribution_type"] = DEFAULT_EVENT_HIT_MULTIPLICITY_DISTRIBUTION_TYPE; 
  defaultSettings["event/hit_multiplicity_distribution_file"] = DEFAULT_EVENT_HIT_MULTIPLICITY_DISTRIBUTION_FILE; 
  defaultSettings["event/hit_multiplicity_gauss_avg"] = DEFAULT_EVENT_HIT_MULTIPLICITY_GAUSS_AVG;
  defaultSettings["event/hit_multiplicity_gauss_stddev"] = DEFAULT_EVENT_HIT_MULTIPLICITY_GAUSS_STDDEV;
  defaultSettings["event/bunch_crossing_rate_ns"] = DEFAULT_EVENT_BUNCH_CROSSING_RATE_NS;
  //@todo Rename to average_trigger_rate_ns?
  defaultSettings["event/average_crossing_rate_ns"] = DEFAULT_EVENT_AVERAGE_CROSSING_RATE_NS;
  defaultSettings["event/trigger_filter_time_ns"] = DEFAULT_EVENT_TRIGGER_FILTER_TIME_NS;
  defaultSettings["event/trigger_filter_enable"] = DEFAULT_EVENT_TRIGGER_FILTER_ENABLE;
  defaultSettings["event/strobe_length_ns"] = DEFAULT_EVENT_STROBE_LENGTH_NS;

  defaultSettings["alpide/region_fifo_size"] = DEFAULT_ALPIDE_REGION_FIFO_SIZE;
  defaultSettings["alpide/region_size"] = DEFAULT_ALPIDE_REGION_SIZE;
  defaultSettings["alpide/pixel_shaping_dead_time_ns"] = DEFAULT_ALPIDE_PIXEL_SHAPING_DEAD_TIME_NS;
  defaultSettings["alpide/pixel_shaping_active_time_ns"] = DEFAULT_ALPIDE_PIXEL_SHAPING_ACTIVE_TIME_NS;  

  
  QStringList simSettingsKeys = readoutSimSettings->allKeys();

  QMap<QString, QString>::iterator i = defaultSettings.begin();
    
  while(i != defaultSettings.end()) {
    //qInfo() << "Key name: " << i.key() << endl;

    
    // Initialize each key missing in readoutSimSettings with a default value
    if(simSettingsKeys.contains(i.key()) == false) {
      //qInfo() << "Key is missing. Updating value" << endl;
      readoutSimSettings->setValue(i.key(), defaultSettings[i.key()]);
    }
    
    i++;
  }  

  return;
}
