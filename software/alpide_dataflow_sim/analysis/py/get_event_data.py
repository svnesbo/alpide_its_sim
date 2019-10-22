import numpy as np
import pandas as pd
import read_settings
import read_trig_action_files


def get_event_data(filename: str) -> pd.DataFrame:
    """Read event data file
    Parameters:
        filename: Full path and name of event_data.csv file
    Return:
        Pandas data frame
    """

    # Open data rate file for current layer/RU
    event_data = pd.read_csv(filename, delimiter=';')

    return event_data


def calc_event_times(event_data: pd.DataFrame, cfg: dict):
    """Calculate time of each event
    Parameters:
        event_data: Pandas data frame for event_data.csv
        cfg: Dict with simulation settings/configuration
    Return:
        None, but modifies the event_data input parameter by adding a new column for event time
    """

    ev_active_start = cfg['alpide']['pixel_shaping_dead_time_ns']
    ev_active_end = cfg['alpide']['pixel_shaping_dead_time_ns'] + cfg['alpide']['pixel_shaping_active_time_ns']

    event_data.insert(0, 'event_time', 0)
    event_data.insert(1, 'event_time_active_start', 0)
    event_data.insert(2, 'event_time_active_end', 0)

    ev_time = 0

    for index_label, row_series in event_data.iterrows():
        event_data.at[index_label, 'event_time'] = ev_time
        event_data.at[index_label, 'event_time_active_start'] = ev_time + ev_active_start
        event_data.at[index_label, 'event_time_active_end'] = ev_time + ev_active_end
        ev_time = ev_time + row_series['delta_t']


def create_trig_strobe_df(event_df: pd.DataFrame, trig_actions_df: pd.DataFrame, cfg: dict) -> dict():
    """Calculate strobe times from event data
    Parameters:
        event_df: Pandas data frame for event_data.csv
        trig_actions_df: Pandas data frame with trigger actions (whether trigger was sent, filtered, not sent due to busy)
        cfg: Dict with simulation settings/configuration
    Return:
        Dict with strobes and time of strobe active, for the triggers that were sent
    """

    trig_strobe_df = trig_actions_df[['trig_id', 'link_0_trig_action']].rename(columns={'link_0_trig_action': 'trig_action'})
    trig_strobe_df['strobe_on_time_ns'] = 0
    trig_strobe_df['strobe_off_time_ns'] = 0

    strobe_active_time = cfg['event']['strobe_active_length_ns']
    strobe_dead_time = cfg['event']['strobe_inactive_length_ns']
    trig_delay = cfg['event']['trigger_delay_ns']

    strobe_on_time_relative = trig_delay + strobe_dead_time
    strobe_off_time_relative = trig_delay + strobe_dead_time + strobe_active_time

    if cfg['simulation']['system_continuous_mode']:
        strobe_period = cfg['simulation']['system_continuous_period_ns']

        for trig_id in trig_strobe_df.index:
            trig_strobe_df.at[trig_id, 'strobe_on_time_ns'] = strobe_period*trig_id + strobe_on_time_relative
            trig_strobe_df.at[trig_id, 'strobe_off_time_ns'] = strobe_period*trig_id + strobe_off_time_relative

    else:
        for trig_id in trig_strobe_df.index:
            trig_strobe_df.at[trig_id, 'strobe_on_time_ns'] = event_df.at[trig_id, 'event_time'] + strobe_on_time_relative
            trig_strobe_df.at[trig_id, 'strobe_off_time_ns'] = event_df.at[trig_id, 'event_time'] + strobe_off_time_relative

    return trig_strobe_df


def calc_strobe_events(event_df: pd.DataFrame, trig_strobe_df: pd.DataFrame, cfg: dict):
    """Calculate which events were included in each strobe
    Parameters:
        event_df: Pandas data frame for event_data.csv
        trig_strobe_df: Dict with strobes and time of strobe active, and trigger info (generated with create_trig_strobe_df()).
                        Event information is appended to this dataframe.
        cfg: Dict with simulation settings/configuration
    Return:
        None, event data is appended to trig_strobe_df
    """

    trig_strobe_df['pileup'] = 0
    trig_strobe_df['event_ids'] = np.empty((len(trig_strobe_df), 0)).tolist()

    evt_df_idx = 0
    evt_df_len = len(event_df.index)

    for trig_id in trig_strobe_df.index:
        strobe_on_time = trig_strobe_df.at[trig_id, 'strobe_on_time_ns']
        strobe_off_time = trig_strobe_df.at[trig_id, 'strobe_off_time_ns']

        pileup = 0
        event_list = []
        while evt_df_idx < evt_df_len:
            evt_on_time = event_df.at[evt_df_idx, 'event_time_active_start']
            evt_off_time = event_df.at[evt_df_idx, 'event_time_active_end']

            # No more events for this strobe
            if evt_on_time > strobe_off_time:
                break

            # Check for two overlapping integer ranges:
            # http://stackoverflow.com/a/12888920
            if(max(strobe_on_time, evt_on_time) <= min(strobe_off_time, evt_off_time)):
                pileup += 1
                event_list.append(evt_df_idx)

            evt_df_idx += 1

        trig_strobe_df.at[trig_id, 'pileup'] = pileup
        trig_strobe_df.at[trig_id, 'event_ids'] = event_list


def get_strobe_multiplicity(event_df: pd.DataFrame, trig_strobe_event_df: pd.DataFrame, cfg: dict, layers: list, chips: list) -> pd.DataFrame:
    """Get pixel hit multiplicities for all strobes for the specified layers and/or chips
    Parameters:
        event_df: Pandas data frame for event_data.csv
        trig_strobe_event_df: Pandas data frame with info on which events are included in which strobes (generated with calc_strobe_events()).
        cfg: Dict with simulation settings/configuration
        layers: List of layers (if any, can be None) to sum hits per strobe
        chips: List of chips (if any, can be None) to sum hits per strobe
    Return:
        Pandas dataframe with multiplicity for events included in each strobe
    """

    strobe_multipl_df = trig_strobe_event_df[['trig_id', 'trig_action', 'event_ids']].copy()

    for layer_id in layers:
        column_name = 'layer_' + str(layer_id)
        strobe_multipl_df[column_name] = np.empty((len(strobe_multipl_df), 0)).tolist()

    for chip_id in chips:
        column_name = 'chip_' + str(chip_id)
        strobe_multipl_df[column_name] = np.empty((len(strobe_multipl_df), 0)).tolist()

    multipl_columns = strobe_multipl_df.columns[3:]

    for strobe_num in trig_strobe_event_df.index:
        strobe_event_list = strobe_multipl_df.at[strobe_num, 'event_ids']

        if len(strobe_event_list) > 0:
            # Iterate over layer/chip columns and retrieve multiplicity for the events for the particular layer/chip
            for col_name in multipl_columns:
                col_multipl_list = []

                # Create list of multiplicites for the events in this column (layer/chip)
                for event_id in strobe_event_list:
                    if event_id < len(event_df.index):
                        col_multipl_list.append(event_df.at[event_id, col_name])

                strobe_multipl_df.at[strobe_num, col_name] = col_multipl_list

    return strobe_multipl_df


if __name__ == '__main__':
    ev_data = get_event_data('C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_11/physics_events_data.csv')

    delta_t = ev_data['delta_t']

    cfg = read_settings.read_settings('C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_11/settings.txt')

    calc_event_times(ev_data, cfg)

    trig_actions = read_trig_action_files.read_trig_actions_file('C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_11/RU_0_0_trigger_actions.dat')

    trig_strobe = create_trig_strobe_df(ev_data, trig_actions, cfg)

    print(trig_actions)
    print(trig_strobe)

    calc_strobe_events(ev_data, trig_strobe, cfg)

    print(ev_data)
    print(trig_strobe)

    strobe_multipl = get_strobe_multiplicity(ev_data, trig_strobe, cfg, [0,1], [1,2,3,4])

    print(strobe_multipl)