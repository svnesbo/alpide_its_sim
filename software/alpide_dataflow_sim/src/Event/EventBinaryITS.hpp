/**
 * @file   EventBinaryITS.hpp
 * @author Simon Voigt Nesbo
 * @date   March 5, 2018
 * @brief  Class for handling events from AliRoot MC simulations for ITS,
 *         stored in a binary data file
 */

#ifndef EVENT_BINARY_ITS_H
#define EVENT_BINARY_ITS_H

#include <QString>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include "EventBaseDiscrete.hpp"


class EventBinaryITS : public EventBaseDiscrete {
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
  EventBinaryITS(Detector::DetectorConfigBase config,
                 Detector::t_global_chip_id_to_position_func global_chip_id_to_position_func,
                 Detector::t_position_to_global_chip_id_func position_to_global_chip_id_func,
                 const QString& path,
                 const QStringList& event_filenames,
                 bool random_event_order = true,
                 int random_seed = 0,
                 bool load_all = false);
};



#endif /* EVENT_BINARY_ITS_H */
