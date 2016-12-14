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


#define ALPIDE_L0_LATENCY_DEFAULT "1965"
#define ALPIDE_LM_LATENCY_DEFAULT "1190"
#define ALPIDE_QED_HANDLE_DEFAULT "true"
#define ALPIDE_CONTINUOUS_ACQ_TIME_DEFAULT "5000"
#define ALPIDE_FORMATTING_TYPE_DEFAULT "0"
#define ALPIDE_FRONT_END_ACTIVE_TIME_DEFAULT "0"
#define ALPIDE_FRONT_END_DELAY_TIME_DEFAULT "10"
#define ALPIDE_GLOBAL_BUSY_DEFAULT "0"
#define ALPIDE_INNER_BANDWIDTH_DEFAULT "3.2"
#define ALPIDE_INPUT_TYPE_DEFAULT "0"
#define ALPIDE_LATCH_ACTIVE_DEFAULT "2000"
#define ALPIDE_LATCH_DELAY_DEFAULT "300"
#define ALPIDE_LATENCY_DEAD_TIME_DEFAULT "10"
#define ALPIDE_MATRIX_LATCH_NUMBER_DEFAULT "10"
#define ALPIDE_MATRIX_READOUT_PERIOD_DEFAULT "0"
#define ALPIDE_NOISE_HANDLE_DEFAULT "2"
#define ALPIDE_NOISE_RATE_DEFAULT "10e-6"
#define ALPIDE_OUTER_BANDWIDTH_DEFAULT "0.32"
#define ALPIDE_OUTER_DAISY_FIFO_SIZE_DEFAULT "128"
#define ALPIDE_PILEUP_HANDLE_DEFAULT "false"
#define ALPIDE_REGION_FIFO_SIZE_DEFAULT "256"
#define ALPIDE_REGION_SIZE_DEFAULT "32"
#define ALPIDE_SENSOR_TYPE_DEFAULT "1"
#define ALPIDE_TIME_SEED_DEFAULT "123456"
#define ALPIDE_TRIGGER_FREQ_DEFAULT "100"

QSettings *getSimSettings(const char *fileName = "settings.txt");
void setDefaultSimSettings(QSettings *readoutSimSettings);


#endif
