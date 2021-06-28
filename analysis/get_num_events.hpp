#ifndef GET_NUM_EVENTS_HPP
#define GET_NUM_EVENTS_HPP

#include <string>

unsigned long get_num_triggered_events_simulated(std::string sim_run_data_path);
unsigned long get_num_untriggered_events_simulated(std::string sim_run_data_path);

#endif
