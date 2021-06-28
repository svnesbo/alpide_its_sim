from read_busy_event_files import *
from get_event_data import *
from read_trig_action_files import *
from itertools import chain
import matplotlib.pyplot as plt

ev_data = get_event_data('C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/test_data/run_0/physics_events_data.csv')
cfg = read_settings.read_settings('C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/test_data/run_0//settings.txt')
#ev_data = get_event_data('C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_23/physics_events_data.csv')
#cfg = read_settings.read_settings('C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_23/settings.txt')
calc_event_times(ev_data, cfg)

trig_actions = read_trig_action_files.read_trig_actions_file('C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/test_data/run_0/RU_0_0_trigger_actions.dat')
#trig_actions = read_trig_action_files.read_trig_actions_file('C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_23/RU_0_0_trigger_actions.dat')
trig_strobe = create_trig_strobe_df(ev_data, trig_actions, cfg)
calc_strobe_events(ev_data, trig_strobe, cfg)
strobe_multipl = get_strobe_multiplicity(ev_data, trig_strobe, cfg, [0, 1], [1, 2, 3, 4])

#busyv_data = read_all_busyv_files('C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_23/', cfg)
#flush_data = read_all_flush_files('C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_23/', cfg)
#abort_data = read_all_abort_files('C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_23/', cfg)
busyv_data = read_all_busyv_files('C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/test_data/run_0/', cfg)
flush_data = read_all_flush_files('C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/test_data/run_0/', cfg)
abort_data = read_all_abort_files('C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/test_data/run_0/', cfg)

layers = [0, 1]
headers = ['trig_id', 'trig_action']

for layer in layers:
    headers.append('layer_' + str(layer))

# Extract the layers we want to plot from strobe multiplicity data,
# and remove triggers that were not sent (current version of simulation the triggers are either sent or filtered,
# and filtering shouldn't lead to data loss as long as the simulation is configured correctly)
selected_strobe_df = strobe_multipl[headers]
filtered_strobe_df = selected_strobe_df[selected_strobe_df['trig_action'] == TrigActions.TRIGGER_SENT]

for layer in layers:
    layer_name = 'layer_' + str(layer)

    layer_busyv_trig_counts = dict()
    layer_flush_trig_counts = dict()
    layer_abort_trig_counts = dict()

    layer_all_affected_trigs = dict()

    # Calculate busyv counts per trigger for this layer
    for entry in busyv_data:
        if entry['layer'] == layer:
            for link in entry['busyv_data']:
                for chip in link['event_data']:
                    for trig in chip['trig_id']:
                        if trig not in layer_busyv_trig_counts:
                            layer_busyv_trig_counts[trig] = 1
                        else:
                            layer_busyv_trig_counts[trig] += 1

    # Calculate flush counts per trigger for this layer
    for entry in flush_data:
        if entry['layer'] == layer:
            for link in entry['flush_data']:
                for chip in link['event_data']:
                    for trig in chip['trig_id']:
                        if trig not in layer_flush_trig_counts:
                            layer_flush_trig_counts[trig] = 1
                        else:
                            layer_flush_trig_counts[trig] += 1

    # Calculate abort counts per trigger for this layer
    for entry in abort_data:
        if entry['layer'] == layer:
            for link in entry['ro_abort_data']:
                for chip in link['event_data']:
                    for trig in chip['trig_id']:
                        if trig not in layer_abort_trig_counts:
                            layer_abort_trig_counts[trig] = 1
                        else:
                            layer_abort_trig_counts[trig] += 1

    layer_busyv_trig_list = list(layer_busyv_trig_counts.keys())
    layer_flush_trig_list = list(layer_flush_trig_counts.keys())
    layer_abort_trig_list = list(layer_abort_trig_counts.keys())

    # Create a list of all triggers that are either affected by busyv, flush, or ro_abort
    layer_all_affected_trigs = layer_busyv_trig_counts.copy()
    layer_all_affected_trigs.update(layer_flush_trig_counts)
    layer_all_affected_trigs.update(layer_abort_trig_counts)
    layer_all_affected_trigs = layer_all_affected_trigs.keys()

    busyv_strobe_df = filtered_strobe_df[filtered_strobe_df['trig_id'].isin(layer_busyv_trig_list)]
    flush_strobe_df = filtered_strobe_df[filtered_strobe_df['trig_id'].isin(layer_flush_trig_list)]
    abort_strobe_df = filtered_strobe_df[filtered_strobe_df['trig_id'].isin(layer_abort_trig_list)]
    all_affected_strobe_df = filtered_strobe_df[filtered_strobe_df['trig_id'].isin(layer_all_affected_trigs)]

    # This gives me multiplicity of _affected_ frames for the specific layer
    # Which is ok, I can plot that. But it does not indicate that all the data was lost for this layer in a frame
    # Maybe I should also sum multiplicity for affected chips for this layer?
    # .. could also be interesting to plot.. maybe even overlay them?
    print(busyv_strobe_df)

    all_affected_strobe_list = list(all_affected_strobe_df[layer_name])

    # Flatten 2D list into 1D list
    all_affected_strobe_list = list(chain.from_iterable(all_affected_strobe_list))

    n, bins, patches = plt.hist(all_affected_strobe_list, bins=100)
    plt.title('Event multiplicity in frames with loss - Layer ' + str(layer))
    plt.xlabel('Multiplicity (pixels)')
    plt.ylabel('Counts')
    plt.show()

    n, bins, patches = plt.hist(all_affected_strobe_list, bins=100, log=True)
    plt.title('Event multiplicity in frames with loss - Layer ' + str(layer))
    plt.xlabel('Multiplicity (pixels)')
    plt.ylabel('Counts')
    plt.show()

print(strobe_multipl)