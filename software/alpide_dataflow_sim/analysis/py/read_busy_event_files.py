import struct

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

                link_data.append({'chip_id': chip_id, 'data': chip_event_data})

            event_data.append({'link_id': data_link_id, 'event_data': link_data})

    return event_data


if __name__ == '__main__':
    data = read_busyv_event_file('C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_11/RU_0_0_busyv_events.dat', False)
    print(data)

    data = read_busy_event_file('C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_11/RU_0_0_busy_events.dat')
    print(data)
