/**
 * @file   parse_cmdline_args.cpp
 * @author Simon Voigt Nesbo
 * @date   November 16, 2017
 * @brief  Header file for command line argument parser
 */

#ifndef PARSE_CMDLINE_ARGS_HPP
#define PARSE_CMDLINE_ARGS_HPP

#include <QSettings>
#include <QCommandLineParser>
#include <QString>

bool parseCommandLine(QCommandLineParser &parser,
                      const QCoreApplication &app,
                      QSettings& settings);

#endif // PARSE_CMDLINE_ARGS_HPP
