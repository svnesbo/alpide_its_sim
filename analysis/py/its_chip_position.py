import pandas as pd


class ITS:
    N_LAYERS = 7
    CHIPS_PER_IB_STAVE    = 9
    CHIPS_PER_HALF_MODULE = 7
    CHIPS_PER_FULL_MODULE = 2*CHIPS_PER_HALF_MODULE
    MODULES_PER_MB_STAVE  = 8
    MODULES_PER_OB_STAVE  = 14
    MODULES_PER_MB_SUB_STAVE = int(MODULES_PER_MB_STAVE/2)
    MODULES_PER_OB_SUB_STAVE = int(MODULES_PER_OB_STAVE/2)
    HALF_MODULES_PER_MB_STAVE = 2*MODULES_PER_MB_STAVE
    HALF_MODULES_PER_OB_STAVE = 2*MODULES_PER_OB_STAVE

    DATA_LINKS_PER_IB_STAVE = 9
    CTRL_LINKS_PER_IB_STAVE = 1

    DATA_LINKS_PER_HALF_MODULE = 1
    CTRL_LINKS_PER_HALF_MODULE = 1

    DATA_LINKS_PER_FULL_MODULE = 2*DATA_LINKS_PER_HALF_MODULE
    CTRL_LINKS_PER_FULL_MODULE = 2*CTRL_LINKS_PER_HALF_MODULE

    STAVES_PER_LAYER = (12, 16, 20, 24, 30, 42, 48)

    # Number of sub staves per stave in a specific layer.
    # Technically there are no sub staves" in the IB staves,
    SUB_STAVES_PER_STAVE = (1, 1, 1, 2, 2, 2, 2)

    STAVE_COUNT_TOTAL = sum(STAVES_PER_LAYER)
    READOUT_UNIT_COUNT = STAVE_COUNT_TOTAL

    # Number of modules per stave in a specific layer.
    # Technically there are no "modules" in the IB staves.
    MODULES_PER_STAVE_IN_LAYER = (1,
                                  1,
                                  1,
                                  MODULES_PER_MB_STAVE,
                                  MODULES_PER_MB_STAVE,
                                  MODULES_PER_OB_STAVE,
                                  MODULES_PER_OB_STAVE)

    # Number of modules per sub stave in a specific layer.
    # Technically there are no "modules" or "sub staves" in the IB staves,
    # but the code that uses it requires it to be 1 and not 0 for inner barrel
    MODULES_PER_SUB_STAVE_IN_LAYER = (1,
                                           1,
                                           1,
                                           MODULES_PER_MB_SUB_STAVE,
                                           MODULES_PER_MB_SUB_STAVE,
                                           MODULES_PER_OB_SUB_STAVE,
                                           MODULES_PER_OB_SUB_STAVE)

    CHIPS_PER_MODULE_IN_LAYER = (CHIPS_PER_IB_STAVE,
                                      CHIPS_PER_IB_STAVE,
                                      CHIPS_PER_IB_STAVE,
                                      CHIPS_PER_FULL_MODULE,
                                      CHIPS_PER_FULL_MODULE,
                                      CHIPS_PER_FULL_MODULE,
                                      CHIPS_PER_FULL_MODULE)

    CHIPS_PER_STAVE_IN_LAYER = (MODULES_PER_STAVE_IN_LAYER[0]*CHIPS_PER_MODULE_IN_LAYER[0],
                                     MODULES_PER_STAVE_IN_LAYER[1]*CHIPS_PER_MODULE_IN_LAYER[1],
                                     MODULES_PER_STAVE_IN_LAYER[2]*CHIPS_PER_MODULE_IN_LAYER[2],
                                     MODULES_PER_STAVE_IN_LAYER[3]*CHIPS_PER_MODULE_IN_LAYER[3],
                                     MODULES_PER_STAVE_IN_LAYER[4]*CHIPS_PER_MODULE_IN_LAYER[4],
                                     MODULES_PER_STAVE_IN_LAYER[5]*CHIPS_PER_MODULE_IN_LAYER[5],
                                     MODULES_PER_STAVE_IN_LAYER[6]*CHIPS_PER_MODULE_IN_LAYER[6])

    CHIPS_PER_LAYER = (STAVES_PER_LAYER[0]*CHIPS_PER_IB_STAVE,
                            STAVES_PER_LAYER[1]*CHIPS_PER_IB_STAVE,
                            STAVES_PER_LAYER[2]*CHIPS_PER_IB_STAVE,
                            STAVES_PER_LAYER[3]*MODULES_PER_MB_STAVE*CHIPS_PER_FULL_MODULE,
                            STAVES_PER_LAYER[4]*MODULES_PER_MB_STAVE*CHIPS_PER_FULL_MODULE,
                            STAVES_PER_LAYER[5]*MODULES_PER_OB_STAVE*CHIPS_PER_FULL_MODULE,
                            STAVES_PER_LAYER[6]*MODULES_PER_OB_STAVE*CHIPS_PER_FULL_MODULE)

    # Number of chips "before" a specific layer
    CUMULATIVE_CHIP_COUNT_AT_LAYER = (0
                                           ,
                                           CHIPS_PER_LAYER[0]
                                           ,
                                           CHIPS_PER_LAYER[0] +
                                           CHIPS_PER_LAYER[1]
                                           ,
                                           CHIPS_PER_LAYER[0] +
                                           CHIPS_PER_LAYER[1] +
                                           CHIPS_PER_LAYER[2]
                                           ,
                                           CHIPS_PER_LAYER[0] +
                                           CHIPS_PER_LAYER[1] +
                                           CHIPS_PER_LAYER[2] +
                                           CHIPS_PER_LAYER[3]
                                           ,
                                           CHIPS_PER_LAYER[0] +
                                           CHIPS_PER_LAYER[1] +
                                           CHIPS_PER_LAYER[2] +
                                           CHIPS_PER_LAYER[3] +
                                           CHIPS_PER_LAYER[4]
                                           ,
                                           CHIPS_PER_LAYER[0] +
                                           CHIPS_PER_LAYER[1] +
                                           CHIPS_PER_LAYER[2] +
                                           CHIPS_PER_LAYER[3] +
                                           CHIPS_PER_LAYER[4] +
                                           CHIPS_PER_LAYER[5]
                                           )

    CHIP_COUNT_TOTAL = sum(CHIPS_PER_LAYER)

    DATA_LINKS_PER_LAYER = (STAVES_PER_LAYER[0]*DATA_LINKS_PER_IB_STAVE,
                                 STAVES_PER_LAYER[1]*DATA_LINKS_PER_IB_STAVE,
                                 STAVES_PER_LAYER[2]*DATA_LINKS_PER_IB_STAVE,
                                 STAVES_PER_LAYER[3]*MODULES_PER_MB_STAVE*DATA_LINKS_PER_FULL_MODULE,
                                 STAVES_PER_LAYER[4]*MODULES_PER_MB_STAVE*DATA_LINKS_PER_FULL_MODULE,
                                 STAVES_PER_LAYER[5]*MODULES_PER_OB_STAVE*DATA_LINKS_PER_FULL_MODULE,
                                 STAVES_PER_LAYER[6]*MODULES_PER_OB_STAVE*DATA_LINKS_PER_FULL_MODULE)


    CTRL_LINKS_PER_LAYER = (STAVES_PER_LAYER[0]*CTRL_LINKS_PER_IB_STAVE,
                                 STAVES_PER_LAYER[1]*CTRL_LINKS_PER_IB_STAVE,
                                 STAVES_PER_LAYER[2]*CTRL_LINKS_PER_IB_STAVE,
                                 STAVES_PER_LAYER[3]*MODULES_PER_MB_STAVE*CTRL_LINKS_PER_FULL_MODULE,
                                 STAVES_PER_LAYER[4]*MODULES_PER_MB_STAVE*CTRL_LINKS_PER_FULL_MODULE,
                                 STAVES_PER_LAYER[5]*MODULES_PER_OB_STAVE*CTRL_LINKS_PER_FULL_MODULE,
                                 STAVES_PER_LAYER[6]*MODULES_PER_OB_STAVE*CTRL_LINKS_PER_FULL_MODULE)


def position_to_global_chip_id(position: dict):
    chip_id = ITS.CUMULATIVE_CHIP_COUNT_AT_LAYER[position['layer_id']]
    chip_id += ITS.CHIPS_PER_STAVE_IN_LAYER[position['layer_id']] * position['stave_id']

    chip_id += position['sub_stave_id'] * ITS.MODULES_PER_SUB_STAVE_IN_LAYER[position['layer_id']] * ITS.CHIPS_PER_MODULE_IN_LAYER[position['layer_id']]

    chip_id += position['module_id'] * ITS.CHIPS_PER_MODULE_IN_LAYER[position['layer_id']]

    chip_id += position['module_chip_id']

    return chip_id


def global_chip_id_to_position(global_chip_id: int):
    layer_id = 0
    sub_stave_id = 0

    while layer_id < ITS.N_LAYERS-1:
        if global_chip_id < ITS.CUMULATIVE_CHIP_COUNT_AT_LAYER[layer_id+1]:
            break
        else:
            layer_id += 1

    chip_num_in_layer = global_chip_id

    if layer_id > 0:
        chip_num_in_layer -= ITS.CUMULATIVE_CHIP_COUNT_AT_LAYER[layer_id]

    stave_id = int(chip_num_in_layer / ITS.CHIPS_PER_STAVE_IN_LAYER[layer_id])
    chip_num_in_stave = chip_num_in_layer % ITS.CHIPS_PER_STAVE_IN_LAYER[layer_id]

    module_id = int(chip_num_in_stave / ITS.CHIPS_PER_MODULE_IN_LAYER[layer_id])
    chip_num_in_module = chip_num_in_stave % ITS.CHIPS_PER_MODULE_IN_LAYER[layer_id]

    # Middle/outer barrel stave? Calculate sub stave id
    if layer_id > 2:
        sub_stave_id = int(module_id / ITS.MODULES_PER_SUB_STAVE_IN_LAYER[layer_id])
        module_id = module_id % ITS.MODULES_PER_SUB_STAVE_IN_LAYER[layer_id]

    position = {'layer_id': layer_id,
                'stave_id': stave_id,
                'sub_stave_id': sub_stave_id,
                'module_id': module_id,
                'module_chip_id': chip_num_in_module}

    return position


def data_link_id_to_sub_stave_and_module_id(data_link_id, layer):
    if layer < 3:
        module_id = 0
        sub_stave_id = 0
    else:
        links_per_stave = int(ITS.DATA_LINKS_PER_LAYER[layer] / ITS.STAVES_PER_LAYER[layer])
        links_per_sub_stave = int(links_per_stave / ITS.SUB_STAVES_PER_STAVE[layer])

        sub_stave_id = int(data_link_id / links_per_sub_stave)

        link_in_sub_stave = data_link_id % links_per_sub_stave

        module_id = int(link_in_sub_stave / ITS.DATA_LINKS_PER_FULL_MODULE)

    return {'sub_stave_id': sub_stave_id, 'module_id': module_id}


def get_sim_layers_and_chips(cfg: dict) -> list:
    """
    Get a dict of layers included in simulation, each with a list of chip ids included in the simulation
    :param cfg: Simulation settings
    :return: Tuple: (list of layers, list of chips)
    """
    staves_in_layer = [cfg['its']['layer0_num_staves'], cfg['its']['layer1_num_staves'],
                       cfg['its']['layer2_num_staves'], cfg['its']['layer3_num_staves'],
                       cfg['its']['layer4_num_staves'], cfg['its']['layer5_num_staves'],
                       cfg['its']['layer6_num_staves']]

    layer_dict = {}

    for layer in range(0,len(staves_in_layer)):
        if staves_in_layer[layer] > 0:
            layer_dict[layer] = {'stave_count': staves_in_layer[layer],
                                 'chips': []}

            #for stave in range(0, staves_in_layer[layer]):
            first_chip_id_in_layer = ITS.CUMULATIVE_CHIP_COUNT_AT_LAYER[layer]

            num_simulated_chips_in_layer = staves_in_layer[layer] * ITS.MODULES_PER_STAVE_IN_LAYER[layer] * ITS.CHIPS_PER_MODULE_IN_LAYER[layer]
            for chip_num in range (0, num_simulated_chips_in_layer):
                layer_dict[layer]['chips'].append(first_chip_id_in_layer+chip_num)

    return layer_dict


if __name__ == '__main__':
    data = list()

    for chip_id in range(0, ITS.CHIP_COUNT_TOTAL):
        chip_position = global_chip_id_to_position(chip_id)
        data.append([chip_id,
                     chip_position['layer_id'],
                     chip_position['stave_id'],
                     chip_position['sub_stave_id'],
                     chip_position['module_id'],
                     chip_position['module_chip_id']])

    df = pd.DataFrame(data, columns=['global_chip_id', 'layer_id', 'stave_id', 'sub_stave_id', 'module_id', 'module_chip_id'])

    # Check that we calculate the same global chip id when going in the reverse direction
    for index, row in df.iterrows():
        global_chip_id = position_to_global_chip_id({'layer_id': row['layer_id'],
                                                         'stave_id': row['stave_id'],
                                                         'sub_stave_id': row['sub_stave_id'],
                                                         'module_id': row['module_id'],
                                                         'module_chip_id': row['module_chip_id']})

        assert global_chip_id == row['global_chip_id'], 'Got chip ID ' + str(global_chip_id) + ' expected ' + row['global_chip_id']

    df.to_csv('ITS_positions.csv', index=None, sep=';')
    print(df)
