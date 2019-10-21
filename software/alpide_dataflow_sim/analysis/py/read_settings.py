import configparser


def read_settings(filename: str):
    """Read settings file for simulation
    Parameters:
        filename: full path of settings file to read
    Return:
        Dict with settings/configuration
    """
    config = configparser.ConfigParser()
    config.read(filename)

    cfg_dict = {s: dict(config.items(s)) for s in config.sections()}


    # Convert values from strings to bool, int and floats

    cfg_dict['alpide']['chip_continuous_mode'] = True if cfg_dict['alpide']['chip_continuous_mode'].lower() == 'true' else False
    cfg_dict['alpide']['data_long_enable'] = True if cfg_dict['alpide']['data_long_enable'].lower() == 'true' else False
    cfg_dict['alpide']['matrix_readout_speed_fast'] = True if cfg_dict['alpide']['matrix_readout_speed_fast'].lower() == 'true' else False
    cfg_dict['alpide']['strobe_extension_enable'] = True if cfg_dict['alpide']['strobe_extension_enable'].lower() == 'true' else False
    cfg_dict['alpide']['dtu_delay'] = int(cfg_dict['alpide']['dtu_delay'])
    cfg_dict['alpide']['minimum_busy_cycles'] = int(cfg_dict['alpide']['minimum_busy_cycles'])
    cfg_dict['alpide']['pixel_shaping_active_time_ns'] = int(cfg_dict['alpide']['pixel_shaping_active_time_ns'])
    cfg_dict['alpide']['pixel_shaping_dead_time_ns'] = int(cfg_dict['alpide']['pixel_shaping_dead_time_ns'])

    cfg_dict['data_output']['write_event_csv'] = True if cfg_dict['data_output']['write_event_csv'].lower() == 'true' else False
    cfg_dict['data_output']['write_vcd'] = True if cfg_dict['data_output']['write_vcd'].lower() == 'true' else False
    cfg_dict['data_output']['write_vcd_clock'] = True if cfg_dict['data_output']['write_vcd_clock'].lower() == 'true' else False
    cfg_dict['data_output']['data_rate_interval_ns'] = int(cfg_dict['data_output']['data_rate_interval_ns'])

    cfg_dict['event']['average_event_rate_ns'] = int(cfg_dict['event']['average_event_rate_ns'])
    cfg_dict['event']['monte_carlo_file_type'] = cfg_dict['event']['monte_carlo_file_type'].lower()
    cfg_dict['event']['qed_noise_input'] = True if cfg_dict['event']['qed_noise_input'].lower() == 'true' else False
    cfg_dict['event']['random_cluster_generation'] = True if cfg_dict['event']['random_cluster_generation'].lower() == 'true' else False
    cfg_dict['event']['random_hit_generation'] = True if cfg_dict['event']['random_hit_generation'].lower() == 'true' else False
    cfg_dict['event']['trigger_filter_enable'] = True if cfg_dict['event']['trigger_filter_enable'].lower() == 'true' else False
    cfg_dict['event']['qed_noise_event_rate_ns'] = int(cfg_dict['event']['qed_noise_event_rate_ns'])
    cfg_dict['event']['qed_noise_feed_rate_ns'] = int(cfg_dict['event']['qed_noise_feed_rate_ns'])
    cfg_dict['event']['random_cluster_size_mean'] = float(cfg_dict['event']['random_cluster_size_mean'])
    cfg_dict['event']['random_cluster_size_stddev'] = float(cfg_dict['event']['random_cluster_size_stddev'])
    cfg_dict['event']['strobe_active_length_ns'] = int(cfg_dict['event']['strobe_active_length_ns'])
    cfg_dict['event']['strobe_inactive_length_ns'] = int(cfg_dict['event']['strobe_inactive_length_ns'])
    cfg_dict['event']['trigger_delay_ns'] = int(cfg_dict['event']['trigger_delay_ns'])
    cfg_dict['event']['trigger_filter_time_ns'] = int(cfg_dict['event']['trigger_filter_time_ns'])

    cfg_dict['its']['bunch_crossing_rate_ns'] = int(cfg_dict['its']['bunch_crossing_rate_ns'])
    cfg_dict['its']['hit_density_layer0'] = float(cfg_dict['its']['hit_density_layer0'])
    cfg_dict['its']['hit_density_layer1'] = float(cfg_dict['its']['hit_density_layer1'])
    cfg_dict['its']['hit_density_layer2'] = float(cfg_dict['its']['hit_density_layer2'])
    cfg_dict['its']['hit_density_layer3'] = float(cfg_dict['its']['hit_density_layer3'])
    cfg_dict['its']['hit_density_layer4'] = float(cfg_dict['its']['hit_density_layer4'])
    cfg_dict['its']['hit_density_layer5'] = float(cfg_dict['its']['hit_density_layer5'])
    cfg_dict['its']['hit_density_layer6'] = float(cfg_dict['its']['hit_density_layer6'])
    cfg_dict['its']['layer0_num_staves'] = int(cfg_dict['its']['layer0_num_staves'])
    cfg_dict['its']['layer1_num_staves'] = int(cfg_dict['its']['layer1_num_staves'])
    cfg_dict['its']['layer2_num_staves'] = int(cfg_dict['its']['layer2_num_staves'])
    cfg_dict['its']['layer3_num_staves'] = int(cfg_dict['its']['layer3_num_staves'])
    cfg_dict['its']['layer4_num_staves'] = int(cfg_dict['its']['layer4_num_staves'])
    cfg_dict['its']['layer5_num_staves'] = int(cfg_dict['its']['layer5_num_staves'])
    cfg_dict['its']['layer6_num_staves'] = int(cfg_dict['its']['layer6_num_staves'])


    cfg_dict['pct']['beam_end_coord_x_mm'] = float(cfg_dict['pct']['beam_end_coord_x_mm'])
    cfg_dict['pct']['beam_end_coord_y_mm'] = float(cfg_dict['pct']['beam_end_coord_y_mm'])
    cfg_dict['pct']['beam_start_coord_x_mm'] = float(cfg_dict['pct']['beam_start_coord_x_mm'])
    cfg_dict['pct']['beam_start_coord_y_mm'] = float(cfg_dict['pct']['beam_start_coord_y_mm'])
    cfg_dict['pct']['beam_step_mm'] = float(cfg_dict['pct']['beam_step_mm'])
    cfg_dict['pct']['beam_time_per_step_us'] = int(cfg_dict['pct']['beam_time_per_step_us'])
    cfg_dict['pct']['layers'] = cfg_dict['pct']['layers'].strip('"').split(";")
    cfg_dict['pct']['num_staves_per_layer'] = int(cfg_dict['pct']['num_staves_per_layer'])
    cfg_dict['pct']['random_beam_stddev_mm'] = float(cfg_dict['pct']['random_beam_stddev_mm'])
    cfg_dict['pct']['random_particles_per_s_mean'] = float(cfg_dict['pct']['random_particles_per_s_mean'])
    cfg_dict['pct']['random_particles_per_s_stddev'] = float(cfg_dict['pct']['random_particles_per_s_stddev'])
    cfg_dict['pct']['time_frame_length_ns'] = int(cfg_dict['pct']['time_frame_length_ns'])

    cfg_dict['simulation']['n_events'] = int(cfg_dict['simulation']['n_events'])
    cfg_dict['simulation']['random_seed'] = int(cfg_dict['simulation']['random_seed'])
    cfg_dict['simulation']['single_chip'] = True if cfg_dict['simulation']['single_chip'].lower() == 'true' else False
    cfg_dict['simulation']['system_continuous_mode'] = True if cfg_dict['simulation']['system_continuous_mode'].lower() == 'true' else False
    cfg_dict['simulation']['system_continuous_period_ns'] = int(cfg_dict['simulation']['system_continuous_period_ns'])
    cfg_dict['simulation']['type'] = cfg_dict['simulation']['type'].lower()

    return cfg_dict

if __name__ == '__main__':
    cfg = read_settings('/home/simon/cernbox/Documents/PhD/CHEP2019/systemc data temp/run_11/settings.txt')
    print(cfg)
