/**
 * @file   main.cpp
 * @author Simon Voigt Nesbo
 * @date   December 11, 2016
 * @brief  Main source file for Alpide Dataflow SystemC simulation testbench
 */


#include "settings/Settings.hpp"
#include "settings/parse_cmdline_args.hpp"
#include "event/EventGenerator.hpp"
#include "Alpide/Alpide.hpp"
#include "stimuli/Stimuli.hpp"
#include "version.hpp"

// Ignore warnings about use of auto_ptr in SystemC library
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <systemc.h>
#pragma GCC diagnostic pop

#include "boost/date_time/posix_time/posix_time.hpp"
#include <set>
#include <iostream>
#include <chrono>
#include <ctime>
#include <QDir>
#include <QFile>
#include <unistd.h>
#include <signal.h>


// Some rough numbers based on a couple of simulation runs
#define EVENT_CSV_KB_PER_EVENT         0.035
#define VCD_TRACES_KB_PER_EVENT       40.000
#define VCD_CLOCK_KB_PER_EVENT         1.500

#define DATA_SIZE_WARNING_MB         512.000


void signal_callback_handler(int signum);
bool create_output_dir(const QSettings* settings, std::string& output_path);
double estimate_data_size(const QSettings* settings);

volatile bool g_terminate_program = false;


int sc_main(int argc, char** argv)
{
  boost::posix_time::ptime simulation_start_time = boost::posix_time::second_clock::local_time();

  std::cout << std::endl; // Print a new line after SystemC info

  // Parse configuration file here
  QSettings* simulation_settings = getSimSettings();

  QCoreApplication app(argc, argv);
  QCoreApplication::setApplicationName("Alpide Dataflow Simulation");
  QCoreApplication::setApplicationVersion(QString::number(VERSION_MAJOR) + "." +
                                          QString::number(VERSION_MINOR));

  QCommandLineParser parser;
  parser.setApplicationDescription("\nAlpide Dataflow Simulation for the upgraded ITS detector");

  // Parse command line arguments. Command line arguments will overwrite
  // the settings specified in the settings file.
  if(parseCommandLine(parser, app, *simulation_settings) == false)
    return 0;

  double sim_data_size = estimate_data_size(simulation_settings);

  std::cout << "Estimated size of simulation data: " << sim_data_size << " kilobytes" << std::endl;
  if((sim_data_size/1024) > DATA_SIZE_WARNING_MB) {
    std::cout << "Warning! Very large files will be generated.. ";

    if(simulation_settings->value("data_output/write_vcd").toBool() == true) {
      std::cout << "Note: VCD file generation is enabled. This will generate lots ";
      std::cout << "of waveform data which is not necessary for analysis" << std::endl;
    }

    std::cout << "Are you sure you want to proceed? [y/N]: ";
    char choice = getchar();
    if(choice != 'y')
      return 0;
  }

  // Create output data directory
  std::string output_dir_str;
  if(create_output_dir(simulation_settings, output_dir_str) == false)
    return 0;

  // Register a signal and signal handler, so that we exit the simulation nicely
  // and not lose data if the user presses CTRL+C on the command line
  signal(SIGINT, signal_callback_handler);

  // Setup SystemC simulation
  Stimuli stimuli("stimuli", simulation_settings, output_dir_str);

  sc_trace_file *wf = NULL;
  sc_core::sc_set_time_resolution(1, sc_core::SC_NS);

  // 25ns period, 0.5 duty cycle, first edge at 2 time units, first value is true
  sc_clock clock_40MHz("clock_40MHz", 25, 0.5, 2, true);

  stimuli.clock(clock_40MHz);

  // Open VCD file
  if(simulation_settings->value("data_output/write_vcd").toBool() == true) {
    std::string vcd_filename = output_dir_str + "/alpide_sim_traces";
    wf = sc_create_vcd_trace_file(vcd_filename.c_str());
    stimuli.addTraces(wf);

    ///@todo Pass vcd trace object to constructor of Stimuli class and Alpide classes?
    if(simulation_settings->value("data_output/write_vcd_clock").toBool() == true) {
      ///@todo Add a warning here if user tries to simulate over 1000 events with this option enabled,
      ///      because it will consume 100s of megabytes
      sc_trace(wf, clock_40MHz, "clock");
    }
  }


  std::cout << "Starting simulation.." << std::endl;

  sc_core::sc_start();

  std::cout << "Started simulation.." << std::endl;

  if(wf != NULL) {
    sc_close_vcd_trace_file(wf);
  }



  boost::posix_time::ptime simulation_end_time  = boost::posix_time::second_clock::local_time();

  boost::posix_time::time_duration diff = simulation_end_time - simulation_start_time;
  diff.total_milliseconds();

  std::cout << "Simulation complete. Elapsed time: " << diff << std::endl;

  return 0;
}


///@brief Callback function for CTRL+C (SIGINT) signal, used for exiting the simulation
/// nicely and not lose data if the user presses CTRL+C on the command line.
void signal_callback_handler(int signum)
{
  std::cout << std::endl << "Caught signal " << signum << ", terminating simulation." << std::endl;

  g_terminate_program = true;
}


///@brief Create output directory based on user settings.
///       The default is "$PWD/sim_output/run_<id>", but a different prefix than
///       sim_output can be specified in settings.
///       A copy of the settings file used for the simulation is created in
///       the simulation output path, as well as a timestamp file.
///@param[in] settings QSettings object which has information about output path prefix
///@param[out] output_path Full path of simulation data output directory
///@return True if creating output directory succeeded, false if not.
bool create_output_dir(const QSettings* settings, std::string& output_path)
{
  auto time_now_ = std::chrono::system_clock::now();
  std::time_t time_now = std::chrono::system_clock::to_time_t(time_now_);
  std::string output_dir_prefix_str;
  std::string output_dir_str;

  output_dir_prefix_str = settings->value("output_dir_prefix").toString().toStdString();

  QDir output_prefix_dir(output_dir_prefix_str.c_str());

  if(output_prefix_dir.exists() == false) {
    if(output_prefix_dir.mkpath(".") == false) {
      std::cout << "Error creating output data prefix path: ";
      std::cout << output_dir_prefix_str << std::endl;
      return false;
    }
  }

  QStringList name_filters;
  name_filters << "run_*";
  output_prefix_dir.setNameFilters(name_filters);
  output_prefix_dir.setSorting(QDir::NoSort);  // will sort manually with std::sort

  auto entryList = output_prefix_dir.entryList();

  if(entryList.length() > 0) {
    // Sort existing files/directories in natural ascending order
    // (ie. 0, 1, 2, 100, as opposed to 0, 1, 100, 2..)
    // https://stackoverflow.com/a/36018397
    QCollator collator;
    collator.setNumericMode(true);
    std::sort(entryList.begin(), entryList.end(), collator);

    QString last_dir = entryList.last();
    last_dir.remove(0, strlen("run_"));
    unsigned int last_run_number = last_dir.toUInt();

    output_dir_str = "run_" + QString::number(last_run_number+1).toStdString();
  } else {
    output_dir_str = "run_0";
  }

  output_dir_str = output_dir_prefix_str + "/" + output_dir_str;

  std::cout << "Output directory for simulation: \"";
  std::cout << output_dir_str << "\"" << std::endl;

  QDir output_dir(output_dir_str.c_str());
  if(output_dir.mkpath(".") == false) {
    std::cerr << "Error creating output data path: " << output_dir_str << std::endl;
    return false;
  }

  // Make a copy of the settings file in the simulation output directory
  std::string output_dir_settings_str = output_dir_str + std::string("/settings.txt");
  QSettings settings_copy(output_dir_settings_str.c_str(), QSettings::IniFormat);
  QStringList settings_keys = settings->allKeys();
  for(auto it = settings_keys.begin(); it != settings_keys.end(); it++) {
    settings_copy.setValue(*it, settings->value(*it));
  }


  // Make a timestamp.txt file here..
  QString timestamp_filename = QString(output_dir_str.c_str()) + QString("/timestamp.txt");
  QFile timestamp_file(timestamp_filename);
  if(!timestamp_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    std::cout << "Error creating timestamp file." << std::endl;
    return false;
  }

  QTextStream out(&timestamp_file);

  out << QString(std::ctime(&time_now));
  out.flush();
  timestamp_file.close();

  output_path = output_dir_str;

  return true;
}


///@brief Estimate how much size the data generated by the simulation will take
///@return Estimated size in kilobytes
double estimate_data_size(const QSettings* settings)
{
  double data_size = 0;
  int num_events = settings->value("simulation/n_events").toInt();


  if(settings->value("data_output/write_event_csv").toBool())
    data_size += EVENT_CSV_KB_PER_EVENT * num_events;

  if(settings->value("data_output/write_vcd").toBool()) {
    data_size += VCD_TRACES_KB_PER_EVENT * num_events;

    if(settings->value("data_output/write_vcd_clock").toBool())
      data_size += VCD_CLOCK_KB_PER_EVENT * num_events;
  }

  return data_size;
}
