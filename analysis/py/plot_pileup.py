from read_busy_event_files import *
from get_event_data import *
from read_trig_action_files import *
from its_chip_position import *
from itertools import chain
import matplotlib
import matplotlib.pyplot as plt
from matplotlib.ticker import (MultipleLocator, FormatStrFormatter,
                               AutoMinorLocator, FuncFormatter)
import pickle
import itertools

def get_pileup(sim_data_path: str, layers: list) -> list:
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

    #pileup_data = dict()

    #for index, row in trig_strobe.iterrows():
    #    if row['pileup'] not in pileup_data:
    #        pileup_data[row['pileup']] = 1
    #    else:
    #        pileup_data[row['pileup']] += 1

    #return pileup_data

    return trig_strobe['pileup'].values


def read_pileup_data(sim_path: str):
    with open(sim_path + '/pileup_data.pkl', 'rb') as f:
        data = pickle.load(f)
    return data

def save_pileup_data(sim_path: str, pileup_data: dict):
    with open(sim_path + '/pileup_data.pkl', "wb") as f:
        pickle.dump(pileup_data, f)

# https://matplotlib.org/examples/pylab_examples/histogram_percent_demo.html
def to_percent(y, position):
    # Ignore the passed in position. This has the effect of scaling the default
    # tick locations.
    s = "{:.1f}".format(100*y)

    # The percent symbol needs escaping in latex
    if matplotlib.rcParams['text.usetex'] is True:
        return s + r'$\%$'
    else:
        return s + '%'


if __name__ == '__main__':
    layers = [0,1,2,3,4,5,6]
    bins = [50, 10, 10, 10, 5, 10, 10]

    # sim_data = [
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_0/', 'title': 'Min-bias/Trig. 100ns', 'evt_rate': 50},
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_4/', 'title': 'Periodic/Trig. 5us/100ns', 'evt_rate': 50},  # Exclude
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_8/', 'title': 'Periodic/Trig. 5us', 'evt_rate': 50},
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_12/', 'title': 'Periodic/Trig. 10us', 'evt_rate': 50},
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_16/', 'title': 'Periodic/Trig. 20us', 'evt_rate': 50},
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_20/', 'title': 'Periodic/Cont. 5us', 'evt_rate': 50},
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_24/', 'title': 'Periodic/Cont. 10us', 'evt_rate': 50},
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_28/', 'title': 'Periodic/Cont. 20us', 'evt_rate': 50},
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_1/', 'title': 'Min-bias/Trig. 100ns', 'evt_rate': 100},
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_5/', 'title': 'Periodic/Trig. 5us/100ns', 'evt_rate': 100},  # Exclude
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_9/', 'title': 'Periodic/Trig. 5us', 'evt_rate': 100},
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_13/', 'title': 'Periodic/Trig. 10us', 'evt_rate': 100},
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_17/', 'title': 'Periodic/Trig. 20us', 'evt_rate': 100},
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_21/', 'title': 'Periodic/Cont. 5us', 'evt_rate': 100},
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_25/', 'title': 'Periodic/Cont. 10us', 'evt_rate': 100},
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_29/', 'title': 'Periodic/Cont. 20us', 'evt_rate': 100},
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_2/', 'title': 'Min-bias/Trig. 100ns', 'evt_rate': 150},
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_6/', 'title': 'Periodic/Trig. 5us/100ns', 'evt_rate': 150},  # Exclude
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_10/', 'title': 'Periodic/Trig. 5us', 'evt_rate': 150},
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_14/', 'title': 'Periodic/Trig. 10us', 'evt_rate': 150},
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_18/', 'title': 'Periodic/Trig. 20us', 'evt_rate': 150},
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_22/', 'title': 'Periodic/Cont. 5us', 'evt_rate': 150},
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_26/', 'title': 'Periodic/Cont. 10us', 'evt_rate': 150},
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_30/', 'title': 'Periodic/Cont. 20us', 'evt_rate': 150},
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_3/', 'title': 'Min-bias/Trig. 100ns', 'evt_rate': 200},
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_7/', 'title': 'Periodic/Trig. 5us/100ns', 'evt_rate': 200},  # Exclude
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_11/', 'title': 'Periodic/Trig. 5us', 'evt_rate': 200},
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_15/', 'title': 'Periodic/Trig. 10us', 'evt_rate': 200},
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_19/', 'title': 'Periodic/Trig. 20us', 'evt_rate': 200},
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_23/', 'title': 'Periodic/Cont. 5us', 'evt_rate': 200},
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_27/', 'title': 'Periodic/Cont. 10us', 'evt_rate': 200},
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_31/', 'title': 'Periodic/Cont. 20us', 'evt_rate': 200}
    # ]

    # 50 kHz
    sim_data = [
        {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_0/', 'title': 'Min-bias/Trig. 100ns', 'evt_rate': 50},
        {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_8/', 'title': 'Periodic/Trig. 5us', 'evt_rate': 50},
        {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_20/', 'title': 'Periodic/Cont. 5us', 'evt_rate': 50},
        {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_12/', 'title': 'Periodic/Trig. 10us', 'evt_rate': 50},
        {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_24/', 'title': 'Periodic/Cont. 10us', 'evt_rate': 50},
        {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_16/', 'title': 'Periodic/Trig. 20us', 'evt_rate': 50},
        {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_28/', 'title': 'Periodic/Cont. 20us', 'evt_rate': 50}
    ]
    plot_output_dir = 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/plots/PbPb/50kHz/'
    additional_title = '50 kHz'

    # 100 kHz
    # sim_data = [
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_1/', 'title': 'Min-bias/Trig. 100ns', 'evt_rate': 100},
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_9/', 'title': 'Periodic/Trig. 5us', 'evt_rate': 100},
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_21/', 'title': 'Periodic/Cont. 5us', 'evt_rate': 100},
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_13/', 'title': 'Periodic/Trig. 10us', 'evt_rate': 100},
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_25/', 'title': 'Periodic/Cont. 10us', 'evt_rate': 100},
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_17/', 'title': 'Periodic/Trig. 20us', 'evt_rate': 100},
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_29/', 'title': 'Periodic/Cont. 20us', 'evt_rate': 100}
    # ]
    # plot_output_dir = 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/plots/PbPb/100kHz/'
    # additional_title = '100 kHz'

    # 150 kHz
    # sim_data = [
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_2/', 'title': 'Min-bias/Trig. 100ns', 'evt_rate': 150},
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_10/', 'title': 'Periodic/Trig. 5us', 'evt_rate': 150},
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_22/', 'title': 'Periodic/Cont. 5us', 'evt_rate': 150},
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_14/', 'title': 'Periodic/Trig. 10us', 'evt_rate': 150},
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_26/', 'title': 'Periodic/Cont. 10us', 'evt_rate': 150},
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_18/', 'title': 'Periodic/Trig. 20us', 'evt_rate': 150},
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_30/', 'title': 'Periodic/Cont. 20us', 'evt_rate': 150}
    # ]
    # plot_output_dir = 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/plots/PbPb/150kHz/'
    # additional_title = '150 kHz'

    # 200 kHz
    # sim_data = [
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_3/', 'title': 'Min-bias/Trig. 100ns', 'evt_rate': 200},
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_11/', 'title': 'Periodic/Trig. 5us', 'evt_rate': 200},
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_23/', 'title': 'Periodic/Cont. 5us', 'evt_rate': 200},
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_15/', 'title': 'Periodic/Trig. 10us', 'evt_rate': 200},
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_27/', 'title': 'Periodic/Cont. 10us', 'evt_rate': 200},
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_19/', 'title': 'Periodic/Trig. 20us', 'evt_rate': 200},
    #     {'path': 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_31/', 'title': 'Periodic/Cont. 20us', 'evt_rate': 200}
    # ]
    # plot_output_dir = 'C:/Users/simon/cernbox/Documents/PhD/CHEP2019/plots/PbPb/200kHz/'
    # additional_title = '200 kHz'

    analyze_raw_data = False
    save_analyzed_data = True
    generate_plots = True
    save_plots = True

    pileup_data = dict()

    if analyze_raw_data:
        for sim in sim_data:
            pileup_data[sim['title']] = get_pileup(sim['path'], layers)

        if save_analyzed_data:
            for sim in sim_data:
                save_pileup_data(sim['path'], pileup_data[sim['title']])
    else:
        for sim in sim_data:
            pileup_data[sim['title']] = read_pileup_data(sim['path'])

    for sim in sim_data:
        pileup_data[sim['title']] = get_pileup(sim['path'], layers)

    if generate_plots:
        # Create the formatter using the function to_percent. This multiplies all the
        # default labels by 100, making them all percentages
        formatter = FuncFormatter(to_percent)

        fig = plt.figure(1, figsize=(10,4))
        plt.style.use('seaborn-whitegrid')

        p_data = list()
        labels = list()
        pileup_max = 0

        for sim in sim_data:
            p_data.append(pileup_data[sim['title']])
            labels.append(sim['title'])
            pileup_max = max(pileup_max, max(pileup_data[sim['title']]))

        plt.hist(p_data,
                 align='left',
                 range=(0, pileup_max+1), bins=pileup_max+1,
                 density=True,
                 histtype='bar', linewidth=1.5, label=labels)

        # Create the formatter using the function to_percent. This multiplies all the
        # default labels by 100, making them all percentages
        formatter = FuncFormatter(to_percent)

        plt.title('Event pileup in readout frames - ' + additional_title, fontsize=22)
        plt.xlabel('Pileup', fontsize=18)
        plt.legend(loc='upper right', prop={'family': 'DejaVu Sans Mono', 'size':14})
        #plt.legend(loc='center left', bbox_to_anchor=(1, 0.5), prop={'family': 'DejaVu Sans Mono', 'size':14})
        ax = fig.gca()
        ax.xaxis.set_major_locator(MultipleLocator(1))

        # Set the percent formatter
        ax.yaxis.set_major_formatter(formatter)

        ax.tick_params(axis='both', which='major', labelsize=16)
        ax.tick_params(axis='both', which='minor', labelsize=16)
        plt.show()

        if save_plots:
            fig.savefig(plot_output_dir + '/pileup.png', bbox_inches='tight', facecolor=fig.get_facecolor(), edgecolor='none')
            fig.savefig(plot_output_dir + '/pileup.pdf', bbox_inches='tight', facecolor=fig.get_facecolor(), edgecolor='none')
            plt.close(fig)