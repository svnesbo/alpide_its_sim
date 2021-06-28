import struct
from enum import IntEnum
import pandas as pd


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

def read_trig_actions_file(filename: str) -> pd.DataFrame:
    """Read a file with trigger actions (sent, filtered, not sent due to busy)
    Parameters:
        filename: full path of filename to read
    Return:
        Pandas dataframe with trigger actions per link
    """

    with open(filename, 'rb') as file:
        file_data = file.read()

        num_triggers = struct.unpack('Q', file_data[0:8])[0]  # uint64_t
        idx = 8

        num_ctrl_links = int(file_data[idx])
        idx += 1

        df_headers = ['trig_id']

        # Note: Only care about first link currently (as they are all the same..)
        #for link_id in range(0, num_ctrl_links):
            #link_str = 'link_' + str(link_id) + '_trig_action'
            #df_headers.append(link_str)
        df_headers.append('link_0_trig_action')

        df_rows = []

        for trig_id in range(0, num_triggers):
            df_row = [trig_id]

            for link_num in range(0, num_ctrl_links):
                trig_action = file_data[idx]
                idx += 1

                # Note: Only care about first link currently (as they are all the same..)
                if link_num == 0:
                    df_row.append(trig_action)

            df_rows.append(df_row)

        trig_actions_df = pd.DataFrame(df_rows, columns=df_headers)

    return trig_actions_df


if __name__ == '__main__':
    data = read_trig_actions_file('C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_1/RU_3_0_trigger_actions.dat')
    print(data)
