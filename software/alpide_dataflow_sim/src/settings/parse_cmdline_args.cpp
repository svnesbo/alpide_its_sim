/**
 * @file   parse_cmdline_args.cpp
 * @author Simon Voigt Nesbo
 * @date   November 16, 2017
 * @brief  Source file for command line argument parser.
 *         Includes command line options for some of options that can be
 *         controlled in the settings file, mainly the ones that were
 *         considered interesting for scripting.
 *         Any value given on the command line will overwrite the corresponding
 *         value from the settings file.
 */

#include "parse_cmdline_args.hpp"
#include <iostream>

///@brief Parse command line options. Settings specified on the command line
///       will overwrite settings in the QSettings object.
///@param parser QCommandLineParser object
///@param app QCoreApplication object
///@param settigns QSettings file, which holds settings for the simulation.
///                These settings are overwritten by settings specifed as
///                command line arguments.
///@return True if parsing is ok and the program can start. Program should not
///        start if false is returned, which indicates that either the user
///        asked for help or version information, or there was an error parsing
///        the command line arguments.
bool parseCommandLine(QCommandLineParser &parser,
                      const QCoreApplication &app,
                      QSettings& settings)
{
  bool start_program = true;
  bool conversion_ok = false;

  parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
  const QCommandLineOption writeVCDOption({"vcd", "write_vcd"},
                                          "Write waveform data to VCD file.");

  const QCommandLineOption writeVCDClockOption({"vcd_clk", "write_vcd_clock"},
                                               "Include clock in VCD file.");

  const QCommandLineOption writeCSVOption({"csv", "write_event_csv"},
                                          "Write event data to Comma Separated Value (CSV) file.");

  const QCommandLineOption singleChipOption({"single", "single_chip"},
                                            "Run in single chip mode, using layer 0 hit density.");

  const QCommandLineOption numEventsOption({"n", "num_events"},
                                           "Number of events to simulate.",
                                           "number of events");

  const QCommandLineOption triggerModeOption({"m", "trigger_mode"},
                                             "Specify \"continuous\" or \"triggered\" mode.",
                                             "mode");

  const QCommandLineOption randomSeedOption({"s", "random_seed"},
                                            "Random seed for random number generators. "
                                            "Use 0 to generate high-entropy random value for seed",
                                            "seed");

  const QCommandLineOption layer0HitDensityOption({"l0", "layer0_hit_density"},
                                                  "Hit density [cm^-2] in layer 0 (or single chip mode).",
                                                  "density");

  const QCommandLineOption layer1HitDensityOption({"l1", "layer1_hit_density"},
                                                  "Hit density [cm^-2] in layer 1.",
                                                  "density");

  const QCommandLineOption layer2HitDensityOption({"l2", "layer2_hit_density"},
                                                  "Hit density [cm^-2] in layer 2.",
                                                  "density");

  const QCommandLineOption layer3HitDensityOption({"l3", "layer3_hit_density"},
                                                  "Hit density [cm^-2] in layer 3.",
                                                  "density");

  const QCommandLineOption layer4HitDensityOption({"l4", "layer4_hit_density"},
                                                  "Hit density [cm^-2] in layer 4.",
                                                  "density");

  const QCommandLineOption layer5HitDensityOption({"l5", "layer5_hit_density"},
                                                  "Hit density [cm^-2] in layer 5.",
                                                  "density");

  const QCommandLineOption layer6HitDensityOption({"l6", "layer6_hit_density"},
                                                  "Hit density [cm^-2] in layer 6.",
                                                  "density");

  const QCommandLineOption avgEventRateOption({"r", "rate"},
                                              "Average event rate (specified in nanoseconds).",
                                              "rate");

  const QCommandLineOption triggerDelayOption({"td", "trig_delay"},
                                              "Trigger delay (specified in nanoseconds).",
                                              "delay");

  const QCommandLineOption triggerFilterTimeOption({"tft", "trig_filter_time"},
                                                   "Trigger filter time (specified in nanoseconds).",
                                                   "filter time");

  const QCommandLineOption triggerFilterOption({"tf", "trig_filtering"},
                                               "Enable trigger filtering.");

  const QCommandLineOption strobeActiveLengthOption({"sa", "strobe_active_time"},
                                                    "Strobe active time (in nanoseconds).",
                                                    "active time");

  const QCommandLineOption strobeInactiveLengthOption({"si", "strobe_inactive_time"},
                                                      "Strobe inactive time (in nanoseconds).",
                                                      "inactive time");

  const QCommandLineOption verboseOption({"V", "verbose"}, "Enable verbose output.");

  const QCommandLineOption outputDirPrefixOption({"o", "output_dir_prefix"},
                                                 "Output directory prefix (default: sim_output/). "
                                                 "Simulations are stored in directories named "
                                                 "\"Run <timestamp>\" in this directory",
                                                 "output_dir_prefix");

  const QCommandLineOption helpOption = parser.addHelpOption();
  const QCommandLineOption versionOption = parser.addVersionOption();

  parser.addOption(writeVCDOption);
  parser.addOption(writeVCDClockOption);
  parser.addOption(writeCSVOption);
  parser.addOption(singleChipOption);
  parser.addOption(numEventsOption);
  parser.addOption(triggerModeOption);
  parser.addOption(randomSeedOption);
  parser.addOption(layer0HitDensityOption);
  parser.addOption(layer1HitDensityOption);
  parser.addOption(layer2HitDensityOption);
  parser.addOption(layer3HitDensityOption);
  parser.addOption(layer4HitDensityOption);
  parser.addOption(layer5HitDensityOption);
  parser.addOption(layer6HitDensityOption);
  parser.addOption(avgEventRateOption);
  parser.addOption(triggerDelayOption);
  parser.addOption(triggerFilterTimeOption);
  parser.addOption(triggerFilterOption);
  parser.addOption(strobeActiveLengthOption);
  parser.addOption(strobeInactiveLengthOption);
  parser.addOption(verboseOption);
  parser.addOption(outputDirPrefixOption);


  // Process the actual command line arguments given by the user
  parser.process(app);

  if (!parser.parse(QCoreApplication::arguments())) {
    std::cout << parser.errorText().toStdString() << std::endl << std::endl;
    parser.showHelp();
    start_program = false;
  }

  if (parser.isSet(helpOption)) {
    parser.showHelp();
    start_program = false;
  } else if (parser.isSet(versionOption)) {
    std::cout << QCoreApplication::applicationName().toStdString();
    std::cout << QCoreApplication::applicationVersion().toStdString();
    std::cout << std::endl;
    start_program = false;
  } else {
    if(parser.isSet(writeVCDOption)) {
      settings.setValue("data_output/write_vcd", "true");
    }

    if(parser.isSet(writeVCDClockOption)) {
      settings.setValue("data_output/write_vcd_clock", "true");
    }

    if(parser.isSet(writeCSVOption)) {
      settings.setValue("data_output/write_event_csv", "true");
    }

    if(parser.isSet(singleChipOption)) {
      settings.setValue("simulation/single_chip", "true");
    }

    if(parser.isSet(numEventsOption)) {
      parser.value(numEventsOption).toULong(&conversion_ok, 10);

      if(conversion_ok == false) {
        std::cout << "Error parsing number of events." << std::endl;
        start_program = false;
      } else {
        settings.setValue("simulation/n_events", parser.value(numEventsOption));
        std::cout << "parser.value(numEventsOption): " << parser.value(numEventsOption).toStdString() << std::endl;
      }
    }

    if(parser.isSet(triggerModeOption)) {
      if(parser.value(triggerModeOption) == "triggered")
        settings.setValue("simulation/continuous_mode", "false");
      else if(parser.value(triggerModeOption) == "continuous")
        settings.setValue("simulation/continuous_mode", "true");
      else {
        std::cout << "Error: Unknown trigger mode \"" << parser.value(triggerModeOption).toStdString();
        std::cout << "\". Specify 'triggered' or 'continuous'." << std::endl;
        start_program = false;
      }
    }

    if(parser.isSet(randomSeedOption)) {
      parser.value(randomSeedOption).toULong(&conversion_ok, 10);

      if(conversion_ok == false) {
        std::cout << "Error parsing random seed." << std::endl;
        start_program = false;
      } else {
        settings.setValue("simulation/random_seed", parser.value(randomSeedOption));
      }
    }

    if(parser.isSet(layer0HitDensityOption)) {
      parser.value(layer0HitDensityOption).toDouble(&conversion_ok);

      if(conversion_ok == false) {
        std::cout << "Error parsing layer 0 hit density." << std::endl;
        start_program = false;
      } else {
        settings.setValue("event/hit_density_layer0", parser.value(layer0HitDensityOption));
      }
    }

    if(parser.isSet(layer1HitDensityOption)) {
      parser.value(layer1HitDensityOption).toDouble(&conversion_ok);

      if(conversion_ok == false) {
        std::cout << "Error parsing layer 1 hit density." << std::endl;
        start_program = false;
      } else {
        settings.setValue("event/hit_density_layer1", parser.value(layer1HitDensityOption));
      }
    }

    if(parser.isSet(layer2HitDensityOption)) {
      parser.value(layer2HitDensityOption).toDouble(&conversion_ok);

      if(conversion_ok == false) {
        std::cout << "Error parsing layer 2 hit density." << std::endl;
        start_program = false;
      } else {
        settings.setValue("event/hit_density_layer2", parser.value(layer2HitDensityOption));
      }
    }

    if(parser.isSet(layer3HitDensityOption)) {
      parser.value(layer3HitDensityOption).toDouble(&conversion_ok);

      if(conversion_ok == false) {
        std::cout << "Error parsing layer 3 hit density." << std::endl;
        start_program = false;
      } else {
        settings.setValue("event/hit_density_layer3", parser.value(layer3HitDensityOption));
      }
    }

    if(parser.isSet(layer4HitDensityOption)) {
      parser.value(layer4HitDensityOption).toDouble(&conversion_ok);

      if(conversion_ok == false) {
        std::cout << "Error parsing layer 4 hit density." << std::endl;
        start_program = false;
      } else {
        settings.setValue("event/hit_density_layer4", parser.value(layer4HitDensityOption));
      }
    }

    if(parser.isSet(layer5HitDensityOption)) {
      parser.value(layer5HitDensityOption).toDouble(&conversion_ok);

      if(conversion_ok == false) {
        std::cout << "Error parsing layer 5 hit density." << std::endl;
        start_program = false;
      } else {
        settings.setValue("event/hit_density_layer5", parser.value(layer5HitDensityOption));
      }
    }

    if(parser.isSet(layer6HitDensityOption)) {
      parser.value(layer6HitDensityOption).toDouble(&conversion_ok);

      if(conversion_ok == false) {
        std::cout << "Error parsing layer 6 hit density." << std::endl;
        start_program = false;
      } else {
        settings.setValue("event/hit_density_layer6", parser.value(layer6HitDensityOption));
      }
    }

    if(parser.isSet(avgEventRateOption)) {
      parser.value(avgEventRateOption).toULong(&conversion_ok, 10);

      if(conversion_ok == false) {
        std::cout << "Error parsing average event rate." << std::endl;
        start_program = false;
      } else {
        settings.setValue("event/average_event_rate_ns", parser.value(avgEventRateOption));
      }
    }

    if(parser.isSet(triggerDelayOption)) {
      parser.value(triggerDelayOption).toULong(&conversion_ok, 10);

      if(conversion_ok == false) {
        std::cout << "Error parsing trigger delay." << std::endl;
        start_program = false;
      } else {
        settings.setValue("event/trigger_delay_ns", parser.value(triggerDelayOption));
      }
    }

    if(parser.isSet(triggerFilterTimeOption)) {
      parser.value(triggerFilterTimeOption).toULong(&conversion_ok, 10);

      if(conversion_ok == false) {
        std::cout << "Error parsing trigger filter time." << std::endl;
        start_program = false;
      } else {
        settings.setValue("event/trigger_filter_time_ns", parser.value(triggerFilterTimeOption));
      }
    }

    if(parser.isSet(triggerFilterOption)) {
      settings.setValue("event/trigger_filter_enable", "true");
    }

    if(parser.isSet(strobeActiveLengthOption)) {
      parser.value(strobeActiveLengthOption).toULong(&conversion_ok, 10);

      if(conversion_ok == false) {
        std::cout << "Error parsing strobe active length." << std::endl;
        start_program = false;
      } else {
        settings.setValue("event/strobe_active_length_ns", parser.value(strobeActiveLengthOption));
      }
    }

    if(parser.isSet(strobeInactiveLengthOption)) {
      parser.value(strobeInactiveLengthOption).toULong(&conversion_ok, 10);

      if(conversion_ok == false) {
        std::cout << "Error parsing strobe inactive length." << std::endl;
        start_program = false;
      } else {
        settings.setValue("event/strobe_inactive_length_ns", parser.value(strobeInactiveLengthOption));
      }
    }

    if(parser.isSet(verboseOption))
      settings.setValue("verbose", "true");
    else
      settings.setValue("verbose", "false");

    if(parser.isSet(outputDirPrefixOption)) {
      if(parser.value(outputDirPrefixOption).length() == 0) {
        std::cout << "Error parsing output directory prefix." << std::endl;
        start_program = false;
      } else {
        settings.setValue("output_dir_prefix", parser.value(outputDirPrefixOption));
      }
    } else {
      settings.setValue("output_dir_prefix", "sim_output");
    }
  }

  return start_program;
}
