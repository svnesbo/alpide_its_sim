from read_busy_event_files import *
from get_event_data import *
from read_trig_action_files import *
from its_chip_position import *
from itertools import chain
import matplotlib.pyplot as plt
import pickle

def get_layer_loss_multiplicity(sim_data_path: str, layers: list) -> list:
    """
    Get lists of multiplicities for frames affected by busyv/flush/abort, one list for each layer.
    Two lists are returned:
    1: This list will have the multiplicity of entire events for the specified layers, regardless of how
       many chips had busyv/flush/abort, and hence regardless of how much data of the event was actually lost.
    2: This list will have the summed multiplicity of lost data for all chips in a layer for a given trigger
       In the case of flush it is not actual amount of data that was flushed that is returned,
       but rather the full multiplicity for that chip (number of flushed pixels is currently not stored by simulation)
    :param sim_data_path: Path to simulation data
    :param layers: List of layers to include
    :return: Tuple of: (List of multiplicities of affected events per layer,
                        list of multiplicity of lost data per layer,
                        list of chips with busyv/flush/abort per trig per layer)
    """
    cfg = read_settings.read_settings(sim_data_path + '/settings.txt')

    layer_dict = get_sim_layers_and_chips(cfg)
    chip_list = []

    for layer in layers:
        assert layer in layer_dict, 'Requested layer was not simulated'
        chip_list.extend(layer_dict[layer]['chips'])

    ev_data = get_event_data(sim_data_path + '/physics_events_data.csv')
    calc_event_times(ev_data, cfg)

    # Todo: don't hardcode RU_0_0, but just find the first one. They should all have the same trigger data in
    #  the current version of the simulation
    trig_actions = read_trig_action_files.read_trig_actions_file(sim_data_path + '/RU_0_0_trigger_actions.dat')
    trig_strobe = create_trig_strobe_df(ev_data, trig_actions, cfg)
    calc_strobe_events(ev_data, trig_strobe, cfg)

    strobe_multipl = get_strobe_multiplicity(ev_data, trig_strobe, cfg, layers, chip_list)

    busyv_data = read_all_busyv_files(sim_data_path, cfg)
    flush_data = read_all_flush_files(sim_data_path, cfg)
    abort_data = read_all_abort_files(sim_data_path, cfg)

    headers = ['trig_id', 'trig_action']

    for layer in layers:
        headers.append('layer_' + str(layer))

    for chip in chip_list:
        headers.append('chip_' + str(chip))

    # Extract the layers we want to plot from strobe multiplicity data,
    # and remove triggers that were not sent (current version of simulation the triggers are either sent or filtered,
    # and filtering shouldn't lead to data loss as long as the simulation is configured correctly)
    selected_strobe_df = strobe_multipl[headers]
    filtered_strobe_df = selected_strobe_df[selected_strobe_df['trig_action'] == TrigActions.TRIGGER_SENT]

    affected_events_multiplicity_list = []
    lost_events_multiplicity_list = []
    busyv_flush_abort_count_list = []
    either_of_busyv_flush_abort_count_list = []

    for layer in layers:
        layer_name = 'layer_' + str(layer)

        layer_busyv_trig_counts = dict()
        layer_flush_trig_counts = dict()
        layer_abort_trig_counts = dict()
        busyv_flush_abort_trig_counts = dict() # Any of them

        layer_all_affected_trigs = dict()

        # Key: trigger, value: list of lost pixels per event that was lost for all chips in layer, for this trigger
        lost_events_multiplicity_dicts = dict()

        # Calculate busyv counts per trigger for this layer
        for entry in busyv_data:
            if entry['layer'] == layer:
                for link in entry['busyv_data']:
                    for chip in link['event_data']:
                        chip_str = 'chip_' + str(chip['global_chip_id'])
                        for trig in chip['trig_id']:
                            # Increase count of busyv for this trigger ID in dict of all triggers where any chip had busyv for this layer
                            if trig not in layer_busyv_trig_counts:
                                layer_busyv_trig_counts[trig] = 1
                                busyv_flush_abort_trig_counts[trig] = 1
                            else:
                                layer_busyv_trig_counts[trig] += 1
                                busyv_flush_abort_trig_counts[trig] += 1

                            if trig not in lost_events_multiplicity_dicts:
                                lost_events_multiplicity_dicts[trig] = list()

                            # Extract the multiplicities for events lost by this busyv, and store them in list for this layer
                            lost_events_multiplicity_dicts[trig].extend(filtered_strobe_df.at[trig, chip_str])


        # Calculate flush counts per trigger for this layer
        for entry in flush_data:
            if entry['layer'] == layer:
                for link in entry['flush_data']:
                    for chip in link['event_data']:
                        chip_str = 'chip_' + str(chip['global_chip_id'])
                        for trig in chip['trig_id']:
                            # Increase count of busyv for this trigger ID in dict of all triggers where any chip had flush for this layer
                            if trig not in layer_flush_trig_counts:
                                layer_flush_trig_counts[trig] = 1
                                busyv_flush_abort_trig_counts[trig] = 1
                            else:
                                layer_flush_trig_counts[trig] += 1
                                busyv_flush_abort_trig_counts[trig] += 1

                            if trig not in lost_events_multiplicity_dicts:
                                lost_events_multiplicity_dicts[trig] = list()

                            # Extract the multiplicities for events lost by this busyv, and store them in list for this layer
                            lost_events_multiplicity_dicts[trig].extend(filtered_strobe_df.at[trig, chip_str])

        # Calculate abort counts per trigger for this layer
        for entry in abort_data:
            if entry['layer'] == layer:
                for link in entry['ro_abort_data']:
                    for chip in link['event_data']:
                        chip_str = 'chip_' + str(chip['global_chip_id'])
                        for trig in chip['trig_id']:
                            # Increase count of busyv for this trigger ID in dict of all triggers where any chip had abort for this layer
                            if trig not in layer_abort_trig_counts:
                                layer_abort_trig_counts[trig] = 1
                                busyv_flush_abort_trig_counts[trig] = 1
                            else:
                                layer_abort_trig_counts[trig] += 1
                                busyv_flush_abort_trig_counts[trig] += 1

                            if trig not in lost_events_multiplicity_dicts:
                                lost_events_multiplicity_dicts[trig] = list()

                            # Extract the multiplicities for events lost by this busyv, and store them in list for this layer
                            lost_events_multiplicity_dicts[trig].extend(filtered_strobe_df.at[trig, chip_str])
                            #if trig in lost_events_multiplicity_dicts[layer]:
                            #    lost_events_multiplicity_dicts[layer][trig].append(filtered_strobe_df.at[trig, chip_str])
                            #else:
                            #    lost_events_multiplicity_dicts[layer][trig] = filtered_strobe_df.at[trig, chip_str]

        layer_busyv_trig_list = list(layer_busyv_trig_counts.keys())
        layer_flush_trig_list = list(layer_flush_trig_counts.keys())
        layer_abort_trig_list = list(layer_abort_trig_counts.keys())

        # Create a list of all triggers that are either affected by busyv, flush, or ro_abort
        layer_all_affected_trigs = layer_busyv_trig_counts.copy()
        layer_all_affected_trigs.update(layer_flush_trig_counts)
        layer_all_affected_trigs.update(layer_abort_trig_counts)
        layer_all_affected_trigs = layer_all_affected_trigs.keys()

        # Strobe data extracted for busyv, flush, or abort:
        #busyv_strobe_df = filtered_strobe_df[filtered_strobe_df['trig_id'].isin(layer_busyv_trig_list)]
        #flush_strobe_df = filtered_strobe_df[filtered_strobe_df['trig_id'].isin(layer_flush_trig_list)]
        #abort_strobe_df = filtered_strobe_df[filtered_strobe_df['trig_id'].isin(layer_abort_trig_list)]

        all_affected_strobe_df = filtered_strobe_df[filtered_strobe_df['trig_id'].isin(layer_all_affected_trigs)]

        affected_multiplicity_for_layer_list = list(all_affected_strobe_df[layer_name])

        # Flatten 2D list into 1D list
        affected_events_multiplicity_list.append(list(chain.from_iterable(affected_multiplicity_for_layer_list)))

        lost_events_multiplicity_list.append(list())

        for trig in lost_events_multiplicity_dicts:
            lost_events_multiplicity_list[layer].append(sum(lost_events_multiplicity_dicts[trig]))

        busyv_flush_abort_count_list.append({'layer': layer,
                                             'busyv_count': layer_busyv_trig_counts,
                                             'flush_count': layer_flush_trig_counts,
                                             'abort_count': layer_abort_trig_counts,
                                             'busyv_flush_abort_count': busyv_flush_abort_trig_counts})

    return (affected_events_multiplicity_list, lost_events_multiplicity_list, busyv_flush_abort_count_list)


def read_affected_multiplicity(sim_path: str):
    with open(sim_path + '/affected_multiplicity.pkl', 'rb') as f:
        data = pickle.load(f)
    return data

def read_loss_multiplicity(sim_path: str):
    with open(sim_path + '/loss_multiplicity.pkl', 'rb') as f:
        data = pickle.load(f)
    return data

def read_busyv_abort_flush_counts(sim_path: str):
    with open(sim_path + '/busyv_abort_flush.pkl', 'rb') as f:
        data = pickle.load(f)
    return data

def save_affected_multiplicity(sim_path: str, aff_multipl_data: dict):
    with open(sim_path + '/affected_multiplicity.pkl', "wb") as f:
        pickle.dump(aff_multipl_data, f)

def save_loss_multiplicity(sim_path: str, loss_multipl_data):
    with open(sim_path + '/loss_multiplicity.pkl', "wb") as f:
        pickle.dump(loss_multipl_data, f)

def save_busyv_abort_flush_counts(sim_path, busyv_abort_flush_data):
    with open(sim_path + '/busyv_abort_flush.pkl', "wb") as f:
        pickle.dump(busyv_abort_flush_data, f)

if __name__ == '__main__':
    layers = [0,1,2,3,4,5,6]
    bins = [100, 10, 10, 10, 5, 10, 10]

    sim_data = [
        {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_0/', 'title': 'MB trig 100ns - 50 kHz'},
        {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_1/', 'title': 'MB trig 100ns- 100 kHz'},
        {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_2/', 'title': 'MB trig 100ns - 150 kHz'},
        {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_3/', 'title': 'MB trig 100ns - 200 kHz'},
        {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_4/', 'title': 'Periodic 5us/trig 100ns - 50 kHz'},
        {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_5/', 'title': 'Periodic 5us/trig 100ns - 100 kHz'},
        {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_6/', 'title': 'Periodic 5us/trig 100ns - 150 kHz'},
        {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_7/', 'title': 'Periodic 5us/trig 100ns - 200 kHz'},
        #{'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_8/', 'title': 'Periodic trig 5us - 50 kHz'},  # Done
        #{'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_9/', 'title': 'Periodic trig 5us - 100 kHz'},  # Done
        {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_10/', 'title': 'Periodic trig 5us - 150 kHz'},
        {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_11/', 'title': 'Periodic trig 5us - 200 kHz'}
        #{'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_12/', 'title': 'Periodic cont 10us - 50 kHz'},
        #{'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_13/', 'title': 'Periodic cont 10us - 100 kHz'},
        #{'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_14/', 'title': 'Periodic cont 10us - 150 kHz'},
        #{'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_15/', 'title': 'Periodic cont 10us - 200 kHz'},
        #{'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_16/', 'title': 'Periodic cont 20us - 50 kHz'},
        #{'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_17/', 'title': 'Periodic cont 20us - 100 kHz'},
        #{'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_18/', 'title': 'Periodic cont 20us - 150 kHz'},
        #{'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_19/', 'title': 'Periodic cont 20us - 200 kHz'},
        #{'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_20/', 'title': 'Periodic cont 5us - 50 kHz'},  # Done
        #{'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_21/', 'title': 'Periodic cont 5us - 100 kHz'},  # Done
        #{'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_22/', 'title': 'Periodic cont 5us - 150 kHz'},  # Done
        #{'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_23/', 'title': 'Periodic cont 5us - 200 kHz'},  # Done
        #{'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_24/', 'title': 'Periodic cont 10us - 50 kHz'},
        #{'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_25/', 'title': 'Periodic cont 10us - 100 kHz'},
        #{'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_26/', 'title': 'Periodic cont 10us - 150 kHz'},
        #{'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_27/', 'title': 'Periodic cont 10us - 200 kHz'},
        #{'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_28/', 'title': 'Periodic cont 20us - 50 kHz'},
        #{'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_29/', 'title': 'Periodic cont 20us - 100 kHz'},
        #{'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_30/', 'title': 'Periodic cont 20us - 150 kHz'},
        #{'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_31/', 'title': 'Periodic cont 20us - 200 kHz'}
    ]

    #sim_data = [
    #    {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/test_data/run_0/', 'title': 'Test data'}
    #]

    analyze_raw_data = False
    save_analyzed_data = True
    generate_plots = True
    save_plots = True
    plot_output_dir = 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/plots/PbPb/'

    affected_multiplicity = dict()
    loss_multiplicity = dict()
    busyv_abort_flush_counts = dict()

    if analyze_raw_data:
        for sim in sim_data:
            affected_multiplicity[sim['title']], \
            loss_multiplicity[sim['title']], \
            busyv_abort_flush_counts[sim['title']] = get_layer_loss_multiplicity(sim['path'], layers)

        if save_analyzed_data:
            for sim in sim_data:
                save_affected_multiplicity(sim['path'], affected_multiplicity[sim['title']])
                save_loss_multiplicity(sim['path'], loss_multiplicity[sim['title']])
                save_busyv_abort_flush_counts(sim['path'], busyv_abort_flush_counts[sim['title']])

    else:
        for sim in sim_data:
            affected_multiplicity[sim['title']] = read_affected_multiplicity(sim['path'])
            loss_multiplicity[sim['title']] = read_loss_multiplicity(sim['path'])
            busyv_abort_flush_counts[sim['title']] = read_busyv_abort_flush_counts(sim['path'])

    if generate_plots:
        for layer in layers:
            # Plot multiplicity of affected events for the whole layer
            # --------------------------------------------------------
            fig = plt.figure(layer)
            plt.style.use('seaborn-whitegrid')

            for sim in sim_data:
                plt.hist(affected_multiplicity[sim['title']][layer], bins=bins[layer], histtype='step', log=True, label=sim['title'])

            plt.title('Full event multiplicity of frames with loss - Layer ' + str(layer))
            plt.xlabel('Multiplicity (pixel hits)')
            plt.ylabel('Counts')
            #plt.legend(loc='lower right')
            plt.legend(loc='center left', bbox_to_anchor=(1, 0.5))
            plt.show()

            if save_plots:
                fig.savefig(plot_output_dir + '/affected_multiplicity_layer' + str(layer) +'.png', bbox_inches='tight', facecolor=fig.get_facecolor(), edgecolor='none')
                fig.savefig(plot_output_dir + '/affected_multiplicity_layer' + str(layer) + '.pdf', bbox_inches='tight', facecolor=fig.get_facecolor(), edgecolor='none')
                plt.close(fig)

            # Plot summed multiplicity for all chips that lost data for an event/trigger for the whole layer
            # ----------------------------------------------------------------------------------------------
            fig = plt.figure(layer)
            plt.style.use('seaborn-whitegrid')

            for sim in sim_data:
                plt.hist(loss_multiplicity[sim['title']][layer], bins=bins[layer], histtype='step', log=True, label=sim['title'])

            plt.title('Lost data multiplicity of frames with loss - Layer ' + str(layer))
            plt.xlabel('Multiplicity (pixel hits)')
            plt.ylabel('Counts')
            plt.legend(loc='center left', bbox_to_anchor=(1, 0.5))
            plt.show()

            if save_plots:
                fig.savefig(plot_output_dir + '/loss_multiplicity_layer' + str(layer) +'.png', bbox_inches='tight', facecolor=fig.get_facecolor(), edgecolor='none')
                fig.savefig(plot_output_dir + '/loss_multiplicity_layer' + str(layer) + '.pdf', bbox_inches='tight', facecolor=fig.get_facecolor(), edgecolor='none')
                plt.close(fig)

            # Plot histogram of number of links with busyv/flush/abort per trigger that has them
            # ----------------------------------------------------------------------------------------------
            fig = plt.figure(layer)
            plt.style.use('seaborn-whitegrid')

            for sim in sim_data:
                num_bins = ITS.CHIPS_PER_STAVE_IN_LAYER[layer]-1
                plt.hist(busyv_abort_flush_counts[sim['title']][layer]['busyv_flush_abort_count'].values(),
                         bins=num_bins, rwidth=1.0, range=(1, num_bins+1), histtype='step', log=False, label=sim['title'])

            plt.title('Number of chips with frame loss - Layer ' + str(layer))
            plt.xlabel('Number of chips')
            plt.ylabel('Counts')
            plt.legend(loc='center left', bbox_to_anchor=(1, 0.5))
            plt.show()

            if save_plots:
                fig.savefig(plot_output_dir + '/chip_frame_loss_hist_layer' + str(layer) + '.png', bbox_inches='tight',
                            facecolor=fig.get_facecolor(), edgecolor='none')
                fig.savefig(plot_output_dir + '/chip_frame_loss_hist_layer' + str(layer) + '.pdf', bbox_inches='tight',
                            facecolor=fig.get_facecolor(), edgecolor='none')
                plt.close(fig)

            # Plot it again with log axis
            fig = plt.figure(layer)
            plt.style.use('seaborn-whitegrid')

            for sim in sim_data:
                num_bins = ITS.CHIPS_PER_STAVE_IN_LAYER[layer] - 1
                plt.hist(busyv_abort_flush_counts[sim['title']][layer]['busyv_flush_abort_count'].values(),
                         bins=num_bins, rwidth=1.0, range=(1, num_bins + 1), histtype='step', log=True,
                         label=sim['title'])

            plt.title('Number of chips with frame loss - Layer ' + str(layer))
            plt.xlabel('Number of chips')
            plt.ylabel('Counts')
            plt.legend(loc='center left', bbox_to_anchor=(1, 0.5))
            plt.show()

            if save_plots:
                fig.savefig(plot_output_dir + '/chip_frame_loss_hist_layer_log' + str(layer) + '.png', bbox_inches='tight',
                            facecolor=fig.get_facecolor(), edgecolor='none')
                fig.savefig(plot_output_dir + '/chip_frame_loss_hist_layer_log' + str(layer) + '.pdf', bbox_inches='tight',
                            facecolor=fig.get_facecolor(), edgecolor='none')
                plt.close(fig)