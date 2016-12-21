/**
 * @file   settings.h
 * @Author Simon Voigt Nesbo <svn@hib.no>
 * @date   November 3, 2016
 * @brief  Header file for simulation settings file handling.
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

#define DEFAULT_EVENT_HIT_MULTIPLICITY_AVG "2000"
#define DEFAULT_EVENT_HIT_MULTIPLICITY_STDDEV "350"
#define DEFAULT_EVENT_BUNCH_CROSSING_RATE_NS "25"
#define DEFAULT_EVENT_AVERAGE_CROSSING_RATE_NS "2500"
#define DEFAULT_EVENT_TRIGGER_FILTER_TIME_NS "10000"
#define DEFAULT_EVENT_TRIGGER_FILTER_ENABLE "true"

#define DEFAULT_ALPIDE_REGION_FIFO_SIZE "256"
#define DEFAULT_ALPIDE_REGION_SIZE "32"


QSettings *getSimSettings(const char *fileName = "settings.txt");
void setDefaultSimSettings(QSettings *readoutSimSettings);


#endif
