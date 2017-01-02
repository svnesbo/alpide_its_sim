# Uses Verilog VCD import found here:
# https://pypi.python.org/pypi/Verilog_VCD/

from Verilog_VCD.Verilog_VCD import parse_vcd
import matplotlib.pyplot as plt
import numpy as np
import pandas


##################################################
# Import data from waveform file here
##################################################

signals = [
    'SystemC.chip_event_buffers_used[7:0]',
    'SystemC.chip_total_number_of_hits[31:0]'
]

vcd = parse_vcd('/home/simon/Code/SystemC/alice-its-alpide-lightweight-systemc/alpide_toy_model/alpide_toy-model_results2.vcd', siglist=signals)
#vcd = parse_vcd('/home/simon/Code/SystemC/alice-its-alpide-lightweight-systemc/alpide_toy_model/alpide_toy-model_results.vcd')

print("Parsing VCD file done.")


nets_value_distributions = dict()

for net_id in vcd:
    net_name = vcd[net_id]['nets'][0]['name']
    net_hier = vcd[net_id]['nets'][0]['hier']
    net_width = vcd[net_id]['nets'][0]['size']
    net_type = vcd[net_id]['nets'][0]['type']


    index = 0
    value_distribution = dict()
    # Iterate through the 'tv' (time/value?) list, where each element is a time and value tuple
    # Calculate the time difference between each entry, and build a dictionary of discrete values and the cumulative
    # times they were valid, which we can plot as a histogram later
    # Note: the tv list is a list of 2-value tuples, where the tuples consist of (time, value)
    for value_pair in vcd[net_id]['tv']:
        time_now = value_pair[0]

        # Get time of next time/value instance, which we will use to calculate the duration of the current value
        if(index+1 < len(vcd[net_id]['tv'])):
            time_next = vcd[net_id]['tv'][index+1][0]
        else:
            time_next = time_now

        time_delta = time_next - time_now

        index += 1

        # Values are stored as a binary string in tv list - convert to integer
        value = int(value_pair[1], 2)

        if value in value_distribution:
            value_distribution[value] += time_delta
        else:
            value_distribution.update({value: time_delta})


    largest_value = sorted(value_distribution.keys())[-1]
    values_x_array = np.arange(0,largest_value+1)
    values_y_array = np.zeros(largest_value+1)

    for key in value_distribution:
        values_y_array[key] = value_distribution[key]

    #nets_value_distributions.update({net_name: value_distribution})
    nets_value_distributions.update({net_name: [values_x_array, values_y_array]})




##################################################
# Import event time and hit multiplicity data here
##################################################
data = pandas.read_csv("../random_data.csv", delimiter=';', header=0)

plt.figure(1)
# Make a normed histogram. It'll be multiplied by 100 later.
#plt.hist(x=nets_value_distributions['chip_total_number_of_hits[31:0]'][0], data=nets_value_distributions['chip_total_number_of_hits[31:0]'][1], bins=50)#, normed=True)
plt.bar(nets_value_distributions['chip_total_number_of_hits[31:0]'][0], nets_value_distributions['chip_total_number_of_hits[31:0]'][1], log=True)
plt.title('Hits in Pixel Matrix')
plt.xlabel('Number of hits')
plt.ylabel('Time [ns]')

plt.figure(2)
plt.bar(nets_value_distributions['chip_event_buffers_used[7:0]'][0], nets_value_distributions['chip_event_buffers_used[7:0]'][1], width=0.5, align='center', log=True)
plt.title('Multi Event Buffer (MEB) size')
plt.xlabel('MEB size')
plt.ylabel('Time [ns]')
plt.xticks(np.arange(min(nets_value_distributions['chip_event_buffers_used[7:0]'][0]), max(nets_value_distributions['chip_event_buffers_used[7:0]'][0])+1, 1.0))

plt.figure(3)
plt.hist(data.delta_t, bins=40, normed=1)
plt.title('Time between events')
plt.xlabel('$\Delta_t$ [ns]')
plt.ylabel('Probability')

plt.figure(4)
plt.hist(data.hit_multiplicity, bins=40, normed=1)
plt.title('Number of hits and multiplicity')
plt.xlabel('Hits')
plt.ylabel('Probability')

plt.show()
