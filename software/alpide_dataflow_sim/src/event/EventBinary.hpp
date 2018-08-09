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
  std::vector<uint8_t> mFileBuffer;
  size_t mFileBufferIdx;

  bool readLayer(std::string event_filename,
                 EventDigits* event);

  void readStave(std::string event_filename,
                 EventDigits* event,
                 std::uint8_t layer_id);

  void readModule(std::string event_filename,
                  EventDigits* event,
                  std::uint8_t layer_id,
                  std::uint8_t stave_id,
                  std::uint8_t sub_stave_id,
                  bool skip);

  void readChip(std::string event_filename,
                EventDigits* event,
                std::uint8_t layer_id,
                std::uint8_t stave_id,
                std::uint8_t sub_stave_id,
                std::uint8_t mod_id,
                bool skip);

  void readEventFiles();
  EventDigits* readEventFile(const QString& event_filename);

public:
  EventBinary(ITS::detectorConfig config,
              const QString& path,
              const QStringList& event_filenames,
              bool random_event_order = true,
              int random_seed = 0,
              bool load_all = false);
};



#endif /* EVENT_DATA_H */
