/**
 * @file   settings.h
 * @Author Simon Voigt Nesbo <svn@hib.no>
 * @date   November 3, 2016
 * @brief  Header file for simulation settings file handling.
 *         This file has definitions for default simulation settings, which can
 *         be used as default values or for generating the settings file if it is missing.
 *
 */

#ifndef _SETTINGS_H
#define _SETTINGS_H

#include <QtCore>
#include <QSettings>
#include <QDir>
#include <QString>


#define DEFAULT_DATA_OUTPUT_WRITE_VCD "true"
#define DEFAULT_DATA_OUTPUT_WRITE_VCD_CLOCK "false"
#define DEFAULT_DATA_OUTPUT_WRITE_EVENT_CSV "true"

#define DEFAULT_SIMULATION_N_CHIPS "25000"
#define DEFAULT_SIMULATION_N_EVENTS "10000"
#define DEFAULT_SIMULATION_CONTINUOUS_MODE "false"
#define DEFAULT_SIMULATION_RANDOM_SEED "0"

#define DEFAULT_EVENT_HIT_MULTIPLICITY_DISTRIBUTION_TYPE "discrete"
#define DEFAULT_EVENT_HIT_MULTIPLICITY_DISTRIBUTION_FILE "multipl_distr_raw_bins.txt"
#define DEFAULT_EVENT_HIT_MULTIPLICITY_GAUSS_AVG "2000"
#define DEFAULT_EVENT_HIT_MULTIPLICITY_GAUSS_STDDEV "350"
#define DEFAULT_EVENT_BUNCH_CROSSING_RATE_NS "25"
#define DEFAULT_EVENT_AVERAGE_CROSSING_RATE_NS "2500"
#define DEFAULT_EVENT_TRIGGER_FILTER_TIME_NS "10000"
#define DEFAULT_EVENT_TRIGGER_FILTER_ENABLE "true"
#define DEFAULT_EVENT_STROBE_LENGTH_NS "5000"

#define DEFAULT_ALPIDE_REGION_FIFO_SIZE "256"
#define DEFAULT_ALPIDE_REGION_SIZE "32"
#define DEFAULT_ALPIDE_PIXEL_SHAPING_DEAD_TIME_NS "200"
#define DEFAULT_ALPIDE_PIXEL_SHAPING_ACTIVE_TIME_NS "6000"


QSettings *getSimSettings(const char *fileName = "settings.txt");
void setDefaultSimSettings(QSettings *readoutSimSettings);


#endif
