/**
 * @file   EventBinary.hpp
 * @author Simon Voigt Nesbo
 * @date   March 5, 2018
 * @brief  Class for handling events from AliRoot MC simulations,
 *         stored in a binary data file
 */

#ifndef EVENT_DATA_H
#define EVENT_DATA_H

#include <QString>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include "EventBase.hpp"


class EventBinary : public EventBase {
private:
  static void readLayer(std::string event_filename,
                        std::ifstream& event_file,
                        EventDigits* event);

  static void readStave(std::string event_filename,
                        std::ifstream& event_file,
                        EventDigits* event,
                        std::uint8_t layer_id);

  static void readModule(std::string event_filename,
                         std::ifstream& event_file,
                         EventDigits* event,
                         std::uint8_t layer_id,
                         std::uint8_t stave_id);

  static void readChip(std::string event_filename,
                       std::ifstream& event_file,
                       EventDigits* event,
                       std::uint8_t layer_id,
                       std::uint8_t stave_id,
                       std::uint8_t mod_id);

public:
  EventBinary(ITS::detectorConfig config, bool random_event_order = true, int random_seed = 0);
  void readEventFiles(const QString& path, const QStringList& event_filenames);
  void readEventFile(const QString& event_filename);
  const EventDigits* getNextEvent(void);
};



#endif /* EVENT_DATA_H */
