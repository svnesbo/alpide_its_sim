import struct
from enum import IntEnum


class TrigActions(IntEnum):
    TRIGGER_SENT = 0
    TRIGGER_NOT_SENT_BUSY = 1
    TRIGGER_FILTERED = 2


# File format for trigger actions file:
#
#   Header:
#   uint64_t: number of triggers
#   uint8_t:  number of control links
#
#   For each trigger ID:
#     Control link 0:
#       uint8_t: link action
#     Control link 1:
#       uint8_t: link action
#     ...
#     Control link n-1:
#       uint8_t: link action

def read_trig_actions_file(filename: str):
    """Read a file with trigger actions (sent, filtered, not sent due to busy)
    Parameters:
        filename: full path of filename to read
    Return:
        List with trigger actions, one entry per link
    """
    trig_actions_data = list()

    with open(filename, 'rb') as file:
        file_data = file.read()

        num_triggers = struct.unpack('Q', file_data[0:8])[0]  # uint64_t
        idx = 8

        num_ctrl_links = int(file_data[idx])
        idx += 1



        for link_id in range(0, num_ctrl_links):
            trig_actions_data.append({'ctrl_link_id': link_id,
                                      'triggers_sent': list(),
                                      'triggers_filtered': list(),
                                      'triggers_not_sent_busy': list(),
                                      'triggers_unknown': list()})

        for trig_id in range(0, num_triggers):
            for link_num in range(0, num_ctrl_links):
                trig_action = file_data[idx]
                idx += 1

                if trig_action == TrigActions.TRIGGER_SENT:
                    trig_actions_data[link_num]['triggers_sent'].append(trig_id)
                elif trig_action == TrigActions.TRIGGER_FILTERED:
                    trig_actions_data[link_num]['triggers_filtered'].append(trig_id)
                elif trig_action == TrigActions.TRIGGER_NOT_SENT_BUSY:
                    trig_actions_data[link_num]['triggers_not_sent_busy'].append(trig_id)
                else:
                    trig_actions_data[link_num]['triggers_unknown'].append(trig_id)

    return trig_actions_data


if __name__ == '__main__':
    data = read_trig_actions_file('C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_1/RU_0_0_trigger_actions.dat')
    print(data)
