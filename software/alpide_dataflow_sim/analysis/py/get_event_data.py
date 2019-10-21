import numpy as np
import pandas as pd  # Should probably have been using pandas and not csv for this from the beginning..
import read_settings


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
    event_data.insert(0, 'event_time_active_start', 1)
    event_data.insert(0, 'event_time_active_end', 2)

    ev_time = 0

    for index_label, row_series in event_data.iterrows():
        event_data.at[index_label, 'event_time'] = ev_time
        event_data.at[index_label, 'event_time_active_start'] = ev_time + ev_active_start
        event_data.at[index_label, 'event_time_active_end'] = ev_time + ev_active_end
        ev_time = ev_time + row_series['delta_t']



# 1. Take trig actions as input
# 2. Take triggered/continuous mode as input
# 3. Take trigger delay as input
# 4. Take strobe length as input
# 5. Points 2. to 4. can probably be input as config settings instead
def get_strobe_times(event_data: pd.DataFrame, trig_actions: list, cfg: dict) -> dict():
    """Calculate strobe times from event data
    Parameters:
        event_data: Pandas data frame for event_data.csv
        trig_actions: List of trigger actions (whether trigger was sent, filtered, not sent due to busy)
        cfg: Dict with simulation settings/configuration
    Return:
        Dict with strobes and time of strobe active, for the triggers that were sent
    """

    strobe_times = dict()

    strobe_active_time = cfg['event']['strobe_active_length_ns']
    strobe_dead_time = cfg['event']['strobe_inactive_length_ns']
    trig_delay = cfg['event']['trigger_delay_ns']

    strobe_on_time_relative = trig_delay + strobe_dead_time
    strobe_off_time_relative = trig_delay + strobe_dead_time + strobe_active_time

    if cfg['simulation']['system_continuous_mode']:
        strobe_period = cfg['simulation']['system_continuous_period_ns']

        # Only checking control link 0 for now
        # Current version of simulation there is no busy handling in RU,
        # so triggers are either sent or filtered, and this should be the same for ALL RUs.
        for trig_id in trig_actions[0]['triggers_sent']:
            strobe_times[trig_id] = {'strobe_on': strobe_period*trig_id + strobe_on_time_relative,
                                     'strobe_off': strobe_period*trig_id + strobe_off_time_relative}

    else:
        event_delta_t = ev_data['delta_t']

    return strobe_times


def get_strobe_multiplicity(layers: list, chips: list, event_data: pd.DataFrame, strobe_times, cfg: dict) -> list():
    """Get pixel hit multiplicities for all strobes, summed for the specified layers and/or chips
    Parameters:
        layers: List of layers (if any, can be None) to sum hits per strobe
        chips: List of chips (if any, can be None) to sum hits per strobe
        event_data: Pandas data frame for event_data.csv
        strobe_times: Dict with strobes and time of strobe active, for the triggers that were sent (generated with get_strobe_times()).
        cfg: Dict with simulation settings/configuration
    Return:
        Dict with strobes and time of strobe active, for the triggers that were sent
    """




if __name__ == '__main__':
    ev_data = get_event_data('/home/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_11/physics_events_data.csv')

    delta_t = ev_data['delta_t']

    cfg = read_settings.read_settings('/home/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_11/settings.txt')

    calc_event_times(ev_data, cfg)

    delta_t = ev_data['delta_t']
    event_times = ev_data['event_time']
    event_times_active_start = ev_data['event_time_active_start']
    event_times_active_end = ev_data['event_time_active_end']

    print(delta_t)
    print(event_times)
    print(event_times_active_start)
    print(event_times_active_end)
