# Uses Verilog VCD import found here:
# https://pypi.python.org/pypi/Verilog_VCD/

from Verilog_VCD.Verilog_VCD import parse_vcd
from Verilog_VCD.Verilog_VCD import get_endtime
import matplotlib.pyplot as plt
import numpy as np
import pandas

from mpl_toolkits.mplot3d import Axes3D



# Takes a time/value pair list as input, calculates the cumulative time for each value,
# and creates a distribution of value vs. cumulative time, as a dictionary where
# value is the key
def create_distribution_from_tv_pair(tv_list, distr_name):
    index = 0
    time_next = 0

    vcd_endtime = get_endtime()

    value_distribution = dict()

    # Iterate through the 'tv' (time/value?) list, where each element is a time and value tuple
    # Calculate the time difference between each entry, and build a dictionary of discrete values and the cumulative
    # times they were valid, which we can plot as a histogram later
    # Note: the tv list is a list of 2-value tuples, where the tuples consist of (time, value)
    for value_pair in tv_list:
        time_now = value_pair[0]

        # Get time of next time/value instance, which we will use to calculate the duration of the current value
        if index + 1 < len(tv_list):
            time_next = tv_list[index + 1][0]
        else:
            # Use simulation end time for the last value
            time_next = vcd_endtime

        time_delta = time_next - time_now

        index += 1

        # Values are stored as a binary string in tv list - convert to integer
        value = int(value_pair[1], 2)

        # Round value to nearest 10
        # value = round(value, -1)

        if value in value_distribution:
            value_distribution[value] += time_delta
        else:
            value_distribution.update({value: time_delta})

    # Create an array with x values from 0 to highest value, and an array with y values
    # corresponding to the index in the x array (matplotlib can't plot the dict directly..)
    largest_value = sorted(value_distribution.keys())[-1]
    values_x_array = np.arange(0, largest_value + 1)
    values_y_array = np.zeros(largest_value + 1)

    # Create the y array
    for key in value_distribution:
        values_y_array[key] = value_distribution[key]

    return {distr_name: [values_x_array, values_y_array]}



#########################################################################
# Parse VCD file here, and set up distributions and ararys with the data
#########################################################################

# signals = [
#     'SystemC.chip_event_buffers_used[7:0]',
#     'SystemC.chip_total_number_of_hits[31:0]'
# ]

#vcd = parse_vcd('/home/simon/Code/SystemC/alice-its-alpide-lightweight-systemc/alpide_toy_model/process/Results/test_data/alpide_toy-model_results.vcd', siglist=signals)
vcd = parse_vcd('/home/simon/Code/SystemC/alice-its-alpide-lightweight-systemc/alpide_toy_model/process/Results/test_data/alpide_toy-model_results.vcd')
physics_events_data = pandas.read_csv("/home/simon/Code/SystemC/alice-its-alpide-lightweight-systemc/alpide_toy_model/process/Results/test_data/physics_events_data.csv", delimiter=';', header=0)
trigger_events_data = pandas.read_csv("/home/simon/Code/SystemC/alice-its-alpide-lightweight-systemc/alpide_toy_model/process/Results/test_data/trigger_events_data.csv", delimiter=';', header=0)

nets_value_distributions = dict()
vcd_net_ids = dict()

alpide_chips_meb_usage = dict()
alpide_chips_meb_usage_distr = dict()
alpide_chips_num_pixels = dict()
alpide_chips_num_pixels_distr = dict()
strobe = dict()
phys_event = dict()

# Extract the signals we are interested in from the vcd object
for net_id in vcd:
    net_name = vcd[net_id]['nets'][0]['name']
    net_hier = vcd[net_id]['nets'][0]['hier']
    net_width = vcd[net_id]['nets'][0]['size']
    net_type = vcd[net_id]['nets'][0]['type']

    if "event_buffers_used" in net_name:
        # Extract chip number from net name
        chip_num = net_name.lstrip("alpide_").split("/")[0]
        alpide_chips_meb_usage.update({chip_num: vcd[net_id]['tv']})

    elif "hits_in_matrix" in net_name:
        # Extract chip number from net name
        chip_num = net_name.lstrip("alpide_").split("/")[0]
        alpide_chips_num_pixels.update({chip_num: vcd[net_id]['tv']})

    elif "STROBE" in net_name:
        strobe = vcd[net_id]['tv']

    elif "PHYSICS_EVENT" in net_name:
        phys_event = vcd[net_id]['tv']

for chip_num in alpide_chips_meb_usage:
    tv_pair = alpide_chips_meb_usage[chip_num]
    alpide_chips_meb_usage_distr.update(create_distribution_from_tv_pair(tv_pair, chip_num))

for chip_num in alpide_chips_num_pixels:
    tv_pair = alpide_chips_num_pixels[chip_num]
    alpide_chips_num_pixels_distr.update(create_distribution_from_tv_pair(tv_pair, chip_num))



#########################################################################
# Parse CSV files here, and set up distributions and ararys with the data
#########################################################################
physics_events_traces = dict()  # Key: chip_id Value: array with number of trace hits per physics event
physics_events_pixels = dict()  # Key: chip_id Value: array with number of pixel hits per physics event
trigger_events_pixels = dict()  # Key: chip_id Value: array with number of pixel hits per trigger event

# Setup arrays for physics events indexed by chip id with hit data
for column_name in physics_events_data:
    # Columns in CSV have names such as chip_101_pixel_hits
    if "pixel_hits" in column_name:
        # Extract chip_id from column name
        chip_id = column_name.split('_')[1]
        physics_events_pixels.update({chip_id: physics_events_data[column_name]})

    elif "trace_hits" in column_name:
        # Extract chip_id from column name
        chip_id = column_name.split('_')[1]
        physics_events_traces.update({chip_id: physics_events_data[column_name]})

# Setup arrays for trigger events indexed by chip id with hit data
for column_name in trigger_events_data:
    if "pixel_hits" in column_name:
        # Columns in CSV have names such as chip_101_pixel_hits
        # Extract chip_id from column name
        chip_id = column_name.split('_')[1]
        trigger_events_pixels.update({chip_id: trigger_events_data[column_name]})


# for net_id in vcd:
#     net_name = vcd[net_id]['nets'][0]['name']
#     net_hier = vcd[net_id]['nets'][0]['hier']
#     net_width = vcd[net_id]['nets'][0]['size']
#     net_type = vcd[net_id]['nets'][0]['type']
#
#     # Keep record of net name vs. net_id, so that we can find the right ID easier
#     vcd_net_ids.update({net_name: net_id})
#
#     index = 0
#     value_distribution = dict()
#     # Iterate through the 'tv' (time/value?) list, where each element is a time and value tuple
#     # Calculate the time difference between each entry, and build a dictionary of discrete values and the cumulative
#     # times they were valid, which we can plot as a histogram later
#     # Note: the tv list is a list of 2-value tuples, where the tuples consist of (time, value)
#     for value_pair in vcd[net_id]['tv']:
#         time_now = value_pair[0]
#
#         # Get time of next time/value instance, which we will use to calculate the duration of the current value
#         if(index+1 < len(vcd[net_id]['tv'])):
#             time_next = vcd[net_id]['tv'][index+1][0]
#         else:
#             # Use simulation end time for the last value
#             time_next = vcd_endtime
#
#         time_delta = time_next - time_now
#
#         index += 1
#
#         # Values are stored as a binary string in tv list - convert to integer
#         value = int(value_pair[1], 2)
#
#         if value in value_distribution:
#             value_distribution[value] += time_delta
#         else:
#             value_distribution.update({value: time_delta})
#
#
#     # Create an array with x values from 0 to highest value, and an array with y values
#     # corresponding to the index in the x array (matplotlib can't plot the dict directly..)
#     largest_value = sorted(value_distribution.keys())[-1]
#     values_x_array = np.arange(0,largest_value+1)
#     values_y_array = np.zeros(largest_value+1)
#
#     # Create the y array
#     for key in value_distribution:
#         values_y_array[key] = value_distribution[key]
#
#     #nets_value_distributions.update({net_name: value_distribution})
#     nets_value_distributions.update({net_name: [values_x_array, values_y_array]})


# Calculate MEB size transitions, ie. how often we go from 0 MEB to 1 MEB, 1 MEB to 2 MEB, 2 MEB to 1 MEB, and so on.
# MEB_size_all_transitions_distribution = dict()     # Distribution of all transitions
# MEB_size_pos_transitions_distribution = dict()     # Distribution of only positive transitions (ie. 0 to 1, not 1 to 0)
# MEB_size_prev_value = -1
# MEB_size_all_transitions_count = 0
# MEB_size_pos_transitions_count = 0
# for value_pair in vcd[vcd_net_ids['chip_event_buffers_used[7:0]']]['tv']:
#     # Values are stored as a binary string in time/value list - convert to integer
#     MEB_size_value = int(value_pair[1], 2)
#
#     # Look for any transition (positive or negative), and store them
#     if MEB_size_value != MEB_size_prev_value:
#         if MEB_size_value in MEB_size_all_transitions_distribution:
#             MEB_size_all_transitions_distribution[MEB_size_value] += 1
#         else:
#             MEB_size_all_transitions_distribution.update({MEB_size_value: 1})
#         MEB_size_all_transitions_count += 1
#
#     if MEB_size_value > MEB_size_prev_value:
#         if MEB_size_value in MEB_size_pos_transitions_distribution:
#             MEB_size_pos_transitions_distribution[MEB_size_value] += 1
#         else:
#             MEB_size_pos_transitions_distribution.update({MEB_size_value: 1})
#         MEB_size_pos_transitions_count += 1
#
#     MEB_size_prev_value = MEB_size_value
#
# # Create an array with x values from 0 to highest value, and an array with y values
# # corresponding to the index in the x array (matplotlib can't plot the dict directly..)
# largest_value_all = sorted(MEB_size_all_transitions_distribution.keys())[-1]
# largest_value_pos = sorted(MEB_size_pos_transitions_distribution.keys())[-1]
# MEB_transition_all_x_array = np.arange(0, largest_value_all + 1)
# MEB_transition_pos_x_array = np.arange(0, largest_value_pos + 1)
# MEB_transition_all_y_array = np.zeros(largest_value_all + 1)
# MEB_transition_pos_y_array = np.zeros(largest_value_pos + 1)
#
# # Create the y-array for all transitions
# for key in MEB_size_all_transitions_distribution:
#     MEB_transition_all_y_array[key] = MEB_size_all_transitions_distribution[key]
#
# # Create the y-array for positive transitions
# for key in MEB_size_pos_transitions_distribution:
#     MEB_transition_pos_y_array[key] = MEB_size_pos_transitions_distribution[key]



fig = plt.figure(1)
ax = fig.add_subplot(111, projection='3d')
#for c, z in zip(['r', 'g', 'b', 'y'], [30, 20, 10, 0]):
z_index = 0
#for chip_num in alpide_chips_num_pixels_distr:
chip_num = 0
while chip_num < 108:
    xarray = alpide_chips_num_pixels_distr[str(chip_num)][0]
    yarray = alpide_chips_num_pixels_distr[str(chip_num)][1]

    # You can provide either a single color or an array. To demonstrate this,
    # the first bar of each set will be colored cyan.
#    cs = [c] * len(xs)
#    cs[0] = 'c'

    z_index += 1
    chip_num += 1

    ax.bar(xarray[1:], yarray[1:], zs=z_index, zdir='y', alpha=0.8, width=1)

ax.set_xlabel('X')
ax.set_ylabel('Y')
ax.set_zlabel('Z')
ax.set_zscale('log')



fig2 = plt.figure(2)
ax2 = fig2.add_subplot(111, projection='3d')
#for c, z in zip(['r', 'g', 'b', 'y'], [30, 20, 10, 0]):
z_index = 0
#for chip_num in alpide_chips_num_pixels_distr:
chip_num = 0
#while chip_num < 108:
while chip_num < 1:
    xarray = alpide_chips_meb_usage_distr[str(chip_num)][0]
    yarray = alpide_chips_meb_usage_distr[str(chip_num)][1]

    # You can provide either a single color or an array. To demonstrate this,
    # the first bar of each set will be colored cyan.
#    cs = [c] * len(xs)
#    cs[0] = 'c'

    z_index += 1
    chip_num += 1

    ax2.bar(xarray, yarray, zs=z_index, zdir='y', alpha=0.8, width=1)

ax2.set_xlabel('X')
ax2.set_ylabel('Y')
ax2.set_zlabel('Z')
ax2.set_zscale('log')







plt.figure(3)
# Make a normed histogram. It'll be multiplied by 100 later.
#plt.hist(x=nets_value_distributions['chip_total_number_of_hits[31:0]'][0], data=nets_value_distributions['chip_total_number_of_hits[31:0]'][1], bins=10)#, normed=True)
#plt.bar(nets_value_distributions['chip_total_number_of_hits[31:0]'][0], nets_value_distributions['chip_total_number_of_hits[31:0]'][1], log=True)
plt.bar(alpide_chips_num_pixels_distr['0'][0], alpide_chips_num_pixels_distr['0'][1], width=1, log=True)
plt.title('Hits in Pixel Matrix - Chip ID 0')
plt.xlabel('Number of hits')
plt.ylabel('Time [ns]')




# TODO: Calculate average number of hits.


# plt.figure(2)
# rects_MEB_plot = plt.bar(nets_value_distributions['chip_event_buffers_used[7:0]'][0], nets_value_distributions['chip_event_buffers_used[7:0]'][1], width=0.5, align='center', log=True)
# plt.title('Multi Event Buffer (MEB) size')
# plt.xlabel('MEB buffers used')
# plt.ylabel('Time [ns]')
# plt.xticks(np.arange(min(nets_value_distributions['chip_event_buffers_used[7:0]'][0]), max(nets_value_distributions['chip_event_buffers_used[7:0]'][0])+1, 1.0))
#
# # Add a percentage on top of the MEB bars. Based on this stackoverflow answer:
# # http://stackoverflow.com/q/36116718
# for rect in rects_MEB_plot:
#     height = rect.get_height()
#     plt.gca().text(rect.get_x() + rect.get_width()/2., 0.99*height,
#             '%d' % int(100*height/vcd_endtime) + "%", ha='center', va='bottom')


# plt.figure(3)
# rects_MEB_plot = plt.bar(MEB_transition_all_x_array, MEB_transition_all_y_array, width=0.5, align='center', log=False)
# plt.title('Multi Event Buffer (MEB) transitions - positive and negative')
# plt.xlabel('MEB buffers used')
# plt.ylabel('Transitions')
# plt.xticks(np.arange(min(MEB_transition_all_x_array), max(MEB_transition_all_x_array)+1, 1.0))

# Add a percentage on top of the MEB bars. Based on this stackoverflow answer:
# http://stackoverflow.com/q/36116718
# for rect in rects_MEB_plot:
#     height = rect.get_height()
#     plt.gca().text(rect.get_x() + rect.get_width()/2., 0.99*height,
#             '%d' % int(100*height/MEB_size_all_transitions_count) + "%", ha='center', va='bottom')


# plt.figure(4)
# rects_MEB_plot = plt.bar(MEB_transition_pos_x_array, MEB_transition_pos_y_array, width=0.5, align='center', log=False)
# plt.title('Multi Event Buffer (MEB) transitions - positive only')
# plt.xlabel('MEB buffers used')
# plt.ylabel('Transitions')
# plt.xticks(np.arange(min(MEB_transition_pos_x_array), max(MEB_transition_pos_x_array)+1, 1.0))

# Add a percentage on top of the MEB bars. Based on this stackoverflow answer:
# http://stackoverflow.com/q/36116718
# for rect in rects_MEB_plot:
#     height = rect.get_height()
#     plt.gca().text(rect.get_x() + rect.get_width()/2., 0.99*height,
#             '%d' % int(100*height/MEB_size_pos_transitions_count) + "%", ha='center', va='bottom')
# # TODO: Calculate how often we go from 0 MEB to 1 MEB, 1 MEB to 2 MEB, 2 MEB to 1 MEB, and so on. And plot that, not how long.



plt.figure(5)
plt.hist(physics_events_data.delta_t, bins=100, normed=1)
plt.title('Time between events')
plt.xlabel('$\Delta_t$ [ns]')
plt.ylabel('Probability')

f, axarr = plt.subplots(2, sharex=True)
axarr[0].hist(physics_events_data.hit_multiplicity, bins=100, normed=1)
axarr[0].set_title('Total number of trace hits and multiplicity - All chips')
axarr[0].set_xlabel('Trace hits')
axarr[0].set_ylabel('Probability')
axarr[1].hist(physics_events_data.hit_multiplicity, bins=100, normed=1)
axarr[1].set_yscale('log')


f, axarr = plt.subplots(2, sharex=True)
axarr[0].hist(physics_events_data.chip_0_trace_hits, bins=100, normed=1)
axarr[0].set_title('Trace hits and multiplicity - Chip ID 0')
axarr[0].set_xlabel('Trace hits')
axarr[0].set_ylabel('Probability')
axarr[1].hist(physics_events_data.chip_0_trace_hits, bins=100, normed=1)
axarr[1].set_yscale('log')



f, axarr = plt.subplots(2, sharex=True)
axarr[0].hist(physics_events_data.chip_0_pixel_hits, bins=100, normed=1)
axarr[0].set_title('Pixel hits and multiplicity - Chip ID 0')
axarr[0].set_xlabel('Pixel hits')
axarr[0].set_ylabel('Probability')
axarr[1].hist(physics_events_data.chip_0_pixel_hits, bins=100, normed=1)
axarr[1].set_yscale('log')


# Plot trace hits for all chips
f, axarr = plt.subplots(2, sharex=True)
for chip_num in physics_events_traces:
    axarr[0].hist(physics_events_traces[chip_num], normed=1, histtype='step', bins=50)
    axarr[0].set_title('Pixel hits and multiplicity - Chip ID ' + chip_num)
    axarr[0].set_xlabel('Pixel hits')
    axarr[0].set_ylabel('Probability')
    axarr[1].hist(physics_events_traces[chip_num], normed=1, histtype='step', stacked=True, bins=50)
    axarr[1].set_yscale('log')

# TODO: Plot aveerage event rate.

#plt.figure(4)
#plt.hist(data.hit_multiplicity, bins=40, normed=1)
#plt.title('Number of hits and multiplicity')
#plt.xlabel('Hits')
#plt.ylabel('Probability')



plt.show()


chip_width = 3
chip_height = 1.5

# Calculate some average numbers etc.
for chip_num in physics_events_traces:
    avg = np.average(physics_events_traces[chip_num])
    hit_density = avg / (chip_width * chip_height)
    print("Chip ID: ", chip_num, "   Average number of traces per event: ", str(avg), "   Hit density: ", hit_density)
