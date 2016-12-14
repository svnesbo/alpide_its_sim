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


/**
   @brief Open a file with simulation settings.
          If the file does not exist, it will be created. If any settings are missing,
          they will be initialized with default values.
          If no filename is specified, the default settings.txt file is used in the
          current directory.
   @param fileName File to open, relative to current directory. Defaults to settings.txt if not supplied.
   @return Pointer to QSettings object initialized with all settings, either from settings file or 
           with default settings if any settings were missing.
*/
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


/**
   @brief Set default settings for each setting that is missing in the QSettings object.
   @param readoutSimSettings Pointer to QSettings object.
*/
void setDefaultSimSettings(QSettings *readoutSimSettings) {
  QMap<QString, QString> defaultSettings;

  // Default settings map
  defaultSettings["alpide/L0_latency_ns"] = ALPIDE_L0_LATENCY_DEFAULT;
  defaultSettings["alpide/LM_latency_ns"] = ALPIDE_LM_LATENCY_DEFAULT;
  defaultSettings["alpide/QED_handle"] = ALPIDE_QED_HANDLE_DEFAULT;
  defaultSettings["alpide/continuous_acq_time_ns"] = ALPIDE_CONTINUOUS_ACQ_TIME_DEFAULT;
  defaultSettings["alpide/formatting_type"] = ALPIDE_FORMATTING_TYPE_DEFAULT;
  defaultSettings["alpide/front_end_active_time_ns"] = ALPIDE_FRONT_END_ACTIVE_TIME_DEFAULT;
  defaultSettings["alpide/front_end_delay_time_ns"] = ALPIDE_FRONT_END_DELAY_TIME_DEFAULT;
  defaultSettings["alpide/global_busy"] = ALPIDE_GLOBAL_BUSY_DEFAULT;
  defaultSettings["alpide/inner_bandwidth_gbit"] = ALPIDE_INNER_BANDWIDTH_DEFAULT;
  defaultSettings["alpide/input_type"] = ALPIDE_INPUT_TYPE_DEFAULT;
  defaultSettings["alpide/latch_active_ns"] = ALPIDE_LATCH_ACTIVE_DEFAULT;
  defaultSettings["alpide/latch_delay_ns"] = ALPIDE_LATCH_DELAY_DEFAULT;
  defaultSettings["alpide/latency_dead_time_ns"] = ALPIDE_LATENCY_DEAD_TIME_DEFAULT;
  defaultSettings["alpide/matrix_latch_number"] = ALPIDE_MATRIX_LATCH_NUMBER_DEFAULT;
  defaultSettings["alpide/matrix_readout_period_ns"] = ALPIDE_MATRIX_READOUT_PERIOD_DEFAULT;
  defaultSettings["alpide/noise_handle"] = ALPIDE_NOISE_HANDLE_DEFAULT;
  defaultSettings["alpide/noise_rate"] = ALPIDE_NOISE_RATE_DEFAULT;
  defaultSettings["alpide/outer_bandwidth_gbit"] = ALPIDE_OUTER_BANDWIDTH_DEFAULT;
  defaultSettings["alpide/outer_daisy_fifo_size"] = ALPIDE_OUTER_DAISY_FIFO_SIZE_DEFAULT;
  defaultSettings["alpide/pileup_handle"] = ALPIDE_PILEUP_HANDLE_DEFAULT;
  defaultSettings["alpide/region_fifo_size"] = ALPIDE_REGION_FIFO_SIZE_DEFAULT;
  defaultSettings["alpide/region_size"] = ALPIDE_REGION_SIZE_DEFAULT;
  defaultSettings["alpide/sensor_type"] = ALPIDE_SENSOR_TYPE_DEFAULT;
  defaultSettings["alpide/time_seed"] = ALPIDE_TIME_SEED_DEFAULT;
  defaultSettings["alpide/trigger_freq_khz"] = ALPIDE_TRIGGER_FREQ_DEFAULT;
  
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
