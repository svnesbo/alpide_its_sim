from read_busy_event_files import *
from get_event_data import *
from read_trig_action_files import *
from itertools import chain
import matplotlib.pyplot as plt

def get_layer_affected_loss_multiplicity(sim_data_path: str, layers: list) -> list:
    """
    Get a list of multiplicities for frames affected by busyv/flush/abort, one list for each layer
    This list will have the multiplicity of entire events for the specified layers, regardless of how
    many chips had busyv/flush/abort, and hence regardless of how much data of the event was actually lost.
    :param sim_data_path: Path to simulation data
    :param layers: List of layers to include
    :return: List of multiplicities per layer
    """

    ev_data = get_event_data(sim_data_path + '/physics_events_data.csv')
    cfg = read_settings.read_settings(sim_data_path + '/settings.txt')

    calc_event_times(ev_data, cfg)

    # Todo: don't hardcode RU_0_0, but just find the first one. They should all have the same trigger data in
    #  the current version of the simulation
    trig_actions = read_trig_action_files.read_trig_actions_file(sim_data_path + '/RU_0_0_trigger_actions.dat')
    trig_strobe = create_trig_strobe_df(ev_data, trig_actions, cfg)
    calc_strobe_events(ev_data, trig_strobe, cfg)
    strobe_multipl = get_strobe_multiplicity(ev_data, trig_strobe, cfg, layers, None)

    busyv_data = read_all_busyv_files(sim_data_path, cfg)
    flush_data = read_all_flush_files(sim_data_path, cfg)
    abort_data = read_all_abort_files(sim_data_path, cfg)

    headers = ['trig_id', 'trig_action']

    for layer in layers:
        headers.append('layer_' + str(layer))

    # Extract the layers we want to plot from strobe multiplicity data,
    # and remove triggers that were not sent (current version of simulation the triggers are either sent or filtered,
    # and filtering shouldn't lead to data loss as long as the simulation is configured correctly)
    selected_strobe_df = strobe_multipl[headers]
    filtered_strobe_df = selected_strobe_df[selected_strobe_df['trig_action'] == TrigActions.TRIGGER_SENT]

    loss_multiplicity_list = []

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

        loss_multiplicity_for_layer_list = list(all_affected_strobe_df[layer_name])

        # Flatten 2D list into 1D list
        loss_multiplicity_list.append(list(chain.from_iterable(loss_multiplicity_for_layer_list)))

    return loss_multiplicity_list


if __name__ == '__main__':
    layers = [0,1,2,3,4,5,6]
    bins = [100, 10, 10, 10, 5, 10, 10]

    sim_data = [
        {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_8/', 'title': 'Periodic trig 5us - 50 kHz'},
        {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_9/', 'title': 'Periodic trig 5us - 100 kHz'},
        {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_10/', 'title': 'Periodic trig 5us - 150 kHz'},
        {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_10/', 'title': 'Periodic trig 5us - 200 kHz'},
        {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_20/', 'title': 'Periodic cont 5us - 50 kHz'},
        {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_21/', 'title': 'Periodic cont 5us - 100 kHz'},
        {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_22/', 'title': 'Periodic cont 5us - 150 kHz'},
        {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_23/', 'title': 'Periodic cont 5us - 200 kHz'}
    ]

    save_plots = True
    plot_output_dir = 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/plots/PbPb/'

    loss_multiplicity = dict()

    for sim in sim_data:
        loss_multiplicity[sim['title']] = get_layer_affected_loss_multiplicity(sim['path'], layers)

    #loss_multiplicity = get_layer_loss_multiplicity('C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/test_data/run_0/', layers)
    #loss_multiplicity = get_layer_loss_multiplicity('C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_23/', layers)


    for layer in layers:
        fig = plt.figure(layer)
        plt.style.use('seaborn-whitegrid')

        for sim in sim_data:
            plt.hist(loss_multiplicity[sim['title']][layer], bins=bins[layer], histtype='step', log=True, label=sim['title'])

        plt.title('Event multiplicity in frames with loss - Layer ' + str(layer))
        plt.xlabel('Multiplicity (pixel hits)')
        plt.ylabel('Counts')
        #plt.legend(loc='lower right')
        plt.legend(loc='center left', bbox_to_anchor=(1, 0.5))
        plt.show()

        if save_plots:
            fig.savefig(plot_output_dir + '/loss_multiplicity_layer' + str(layer) +'.png', bbox_inches='tight', facecolor=fig.get_facecolor(), edgecolor='none')
            fig.savefig(plot_output_dir + '/loss_multiplicity_layer' + str(layer) + '.pdf', bbox_inches='tight', facecolor=fig.get_facecolor(), edgecolor='none')
            plt.close(fig)

        #n, bins, patches = plt.hist(loss_multiplicity[layer], bins=100, log=True)
        #plt.title('Event multiplicity in frames with loss - Layer ' + str(layer))
        #plt.xlabel('Multiplicity (pixels)')
        #plt.ylabel('Counts')
        #plt.show()

    #plt.show()