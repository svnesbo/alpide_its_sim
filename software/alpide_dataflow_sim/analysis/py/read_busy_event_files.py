import struct
import read_settings

# File format for busy event file:
#
#   Header:
#   uint8_t:  number of data links
#
#   For each data link:
#       Header:
#          uint64_t: Number of "busy events"
#       Data (for each busy event)
#          uint64_t: time of BUSY_ON
#          uint64_t: time of BUSY_OFF
#          uint64_t: trigger ID when BUSY_ON occured
#          uint64_t: trigger ID when BUSY_OFF occured
def read_busy_event_file(filename: str):
    """Read a file with busy on/off events.
    Parameters:
        filename: full path of filename to read
    Return:
        List with busy on/off data, on entry per link
    """
    event_data = list()

    with open(filename, 'rb') as file:
        file_data = file.read()

        num_data_links = int(file_data[0])
        idx = 1

        for data_link_id in range(0, num_data_links):
            link_data = list()

            num_events = struct.unpack('Q', file_data[idx:idx+8])[0] # uint64_t
            idx += 8

            for event_num in range(0, num_events):
                busy_on_time_ns = struct.unpack('Q', file_data[idx:idx+8])[0]  # uint64_t
                idx += 8

                busy_off_time_ns = struct.unpack('Q', file_data[idx:idx + 8])[0]  # uint64_t
                idx += 8

                busy_on_trig_id = struct.unpack('Q', file_data[idx:idx + 8])[0]  # uint64_t
                idx += 8

                busy_off_trig_id = struct.unpack('Q', file_data[idx:idx + 8])[0]  # uint64_t
                idx += 8

                link_data.append({'busy_on_time_ns': busy_on_time_ns,
                                  'busy_off_time_ns': busy_off_time_ns,
                                  'busy_on_trig_id': busy_on_trig_id,
                                  'busy_off_trig_id': busy_off_trig_id})

            event_data.append({'link_id': data_link_id, 'event_data': link_data})

    return event_data


# File format for busyv/flush/abort/fatal event files:
#
#   Header:
#   uint8_t:  number of data links
#
#   For each data link:
#       uint8_t: Number of chips with data for this link
#
#       For each chip in data link which has data:
#          Header:
#             uint8_t:  Chip ID
#             uint64_t: Number of "readout abort events"
#          Data (for each trigger chip was in readout abort)
#             uint64_t: trigger ID for readout abort event
def read_busyv_event_file(filename: str, inner_barrel: bool):
    """Read a file with busy violation, flush incomplete, readout abort or fatal events. Same file format is used for all of those files.
    Parameters:
        filename: full path of filename to read
        inner_barrel: True if file is for inner barrel chips.
                      For outer barrel the lower 3 bits of chip ID is masked out
                      so we are left with local chip id within the link
    Return:
        List with event data, one entry per link. Each link entry contains another link with entries per chip that
        contain the actual busyv/flush/abort/fatal events.
    """
    event_data = list()

    with open(filename, 'rb') as file:
        file_data = file.read()

        num_data_links = int(file_data[0])
        idx = 1

        for data_link_id in range(0, num_data_links):
            link_data = list()

            num_chips_with_data = int(file_data[idx])
            idx += 1

            print('num chips with data: ', num_chips_with_data)

            for chip_num in range(0,num_chips_with_data):
                chip_event_data = list()

                chip_id = int(file_data[idx])
                idx += 1

                print('chip id: ', chip_id)

                if not inner_barrel:
                    chip_id = chip_id & 0x7

                num_events = struct.unpack('Q', file_data[idx:idx+8])[0] # uint64_t
                idx += 8

                for event_num in range(0, num_events):
                    event_trig_num = struct.unpack('Q', file_data[idx:idx+8])[0]  # uint64_t
                    idx += 8

                    chip_event_data.append(event_trig_num)

                link_data.append({'chip_id': chip_id, 'trig_id': chip_event_data})

            event_data.append({'link_id': data_link_id, 'event_data': link_data})

    return event_data


def read_all_busy_files(sim_data_path: str, cfg: dict) -> list:
    file_list = list()
    busy_list = list()

    if cfg['simulation']['type'] == 'its':
        for layer in range(0,7):
            num_staves_in_layer = cfg['its']['layer' + str(layer) + '_num_staves']

            for stave in range(0,num_staves_in_layer):
                filename_str = sim_data_path + '/RU_' + str(layer) + '_' + str(stave) + '_busy_events.dat'
                if layer < 3:
                    file_list.append({'layer': layer, 'stave': stave, 'inner_barrel': True, 'filename': filename_str})
                else:
                    file_list.append({'layer': layer, 'stave': stave, 'inner_barrel': False, 'filename': filename_str})
    else:
        raise NotImplementedError('Reading busyv files only implemented for ITS at the moment.')

    for file_entry in file_list:
        busy_data = read_busy_event_file(file_entry['filename'])
        busy_list.append({'layer': file_entry['layer'],
                          'stave': file_entry['stave'],
                          'inner_barrel': file_entry['inner_barrel'],
                          'busy_data': busy_data})

    return busy_list

def _read_all_busyv_files(sim_data_path: str, filetype: str, cfg: dict) -> list:
    busyv_list = list()
    file_list = list()

    if cfg['simulation']['type'] == 'its':
        for layer in range(0,7):
            num_staves_in_layer = cfg['its']['layer' + str(layer) + '_num_staves']

            for stave in range(0,num_staves_in_layer):
                filename_str = sim_data_path + '/RU_' + str(layer) + '_' + str(stave) + '_' + filetype + '_events.dat'
                if layer < 3:
                    file_list.append({'layer': layer, 'stave': stave, 'inner_barrel': True, 'filename': filename_str})
                else:
                    file_list.append({'layer': layer, 'stave': stave, 'inner_barrel': False, 'filename': filename_str})
    else:
        raise NotImplementedError('Reading busyv files only implemented for ITS at the moment.')

    for file_entry in file_list:
        busyv_data = read_busyv_event_file(file_entry['filename'], file_entry['inner_barrel'])
        data_key = filetype + '_data'
        busyv_list.append({'layer': file_entry['layer'],
                           'stave': file_entry['stave'],
                           'inner_barrel': file_entry['inner_barrel'],
                           data_key: busyv_data})

    return busyv_list

def read_all_busyv_files(sim_data_path: str, cfg: dict) -> list:
    return _read_all_busyv_files(sim_data_path, 'busyv', cfg)

def read_all_flush_files(sim_data_path: str, cfg: dict) -> list:
    return _read_all_busyv_files(sim_data_path, 'flush', cfg)

def read_all_abort_files(sim_data_path: str, cfg: dict) -> list:
    return _read_all_busyv_files(sim_data_path, 'ro_abort', cfg)

def read_all_fatal_files(sim_data_path: str, cfg: dict) -> list:
    return _read_all_busyv_files(sim_data_path, 'fatal', cfg)


if __name__ == '__main__':
    cfg = read_settings.read_settings('C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_11/settings.txt')

    busy_data = read_all_busy_files('C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_11/', cfg)
    busyv_data = read_all_busyv_files('C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_11/', cfg)

    print('asdf')
    #busyv_data = read_busyv_event_file('C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_11/RU_0_0_busyv_events.dat', False)
    #print(busyv_data)

    #busy_data = read_busy_event_file('C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_11/RU_0_0_busy_events.dat')
    #print(busy_data)
