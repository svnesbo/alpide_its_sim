import pandas as pd


class ITS:
    def __init__(self):
        self.N_LAYERS = 7
        self.CHIPS_PER_IB_STAVE    = 9
        self.CHIPS_PER_HALF_MODULE = 7
        self.CHIPS_PER_FULL_MODULE = 2*self.CHIPS_PER_HALF_MODULE
        self.MODULES_PER_MB_STAVE  = 8
        self.MODULES_PER_OB_STAVE  = 14
        self.MODULES_PER_MB_SUB_STAVE = int(self.MODULES_PER_MB_STAVE/2)
        self.MODULES_PER_OB_SUB_STAVE = int(self.MODULES_PER_OB_STAVE/2)
        self.HALF_MODULES_PER_MB_STAVE = 2*self.MODULES_PER_MB_STAVE
        self.HALF_MODULES_PER_OB_STAVE = 2*self.MODULES_PER_OB_STAVE

        self.DATA_LINKS_PER_IB_STAVE = 9
        self.CTRL_LINKS_PER_IB_STAVE = 1

        self.DATA_LINKS_PER_HALF_MODULE = 1
        self.CTRL_LINKS_PER_HALF_MODULE = 1

        self.DATA_LINKS_PER_FULL_MODULE = 2*self.DATA_LINKS_PER_HALF_MODULE
        self.CTRL_LINKS_PER_FULL_MODULE = 2*self.CTRL_LINKS_PER_HALF_MODULE

        self.STAVES_PER_LAYER = (12, 16, 20, 24, 30, 42, 48)

        # Number of sub staves per stave in a specific layer.
        # Technically there are no sub staves" in the IB staves,
        self.SUB_STAVES_PER_STAVE = (1, 1, 1, 2, 2, 2, 2)

        self.STAVE_COUNT_TOTAL = sum(self.STAVES_PER_LAYER)
        self.READOUT_UNIT_COUNT = self.STAVE_COUNT_TOTAL

        # Number of modules per stave in a specific layer.
        # Technically there are no "modules" in the IB staves.
        self.MODULES_PER_STAVE_IN_LAYER = (1,
                                           1,
                                           1,
                                           self.MODULES_PER_MB_STAVE,
                                           self.MODULES_PER_MB_STAVE,
                                           self.MODULES_PER_OB_STAVE,
                                           self.MODULES_PER_OB_STAVE)

        # Number of modules per sub stave in a specific layer.
        # Technically there are no "modules" or "sub staves" in the IB staves,
        # but the code that uses it requires it to be 1 and not 0 for inner barrel
        self.MODULES_PER_SUB_STAVE_IN_LAYER = (1,
                                               1,
                                               1,
                                               self.MODULES_PER_MB_SUB_STAVE,
                                               self.MODULES_PER_MB_SUB_STAVE,
                                               self.MODULES_PER_OB_SUB_STAVE,
                                               self.MODULES_PER_OB_SUB_STAVE)

        self.CHIPS_PER_MODULE_IN_LAYER = (self.CHIPS_PER_IB_STAVE,
                                          self.CHIPS_PER_IB_STAVE,
                                          self.CHIPS_PER_IB_STAVE,
                                          self.CHIPS_PER_FULL_MODULE,
                                          self.CHIPS_PER_FULL_MODULE,
                                          self.CHIPS_PER_FULL_MODULE,
                                          self.CHIPS_PER_FULL_MODULE)

        self.CHIPS_PER_STAVE_IN_LAYER = (self.MODULES_PER_STAVE_IN_LAYER[0]*self.CHIPS_PER_MODULE_IN_LAYER[0],
                                         self.MODULES_PER_STAVE_IN_LAYER[1]*self.CHIPS_PER_MODULE_IN_LAYER[1],
                                         self.MODULES_PER_STAVE_IN_LAYER[2]*self.CHIPS_PER_MODULE_IN_LAYER[2],
                                         self.MODULES_PER_STAVE_IN_LAYER[3]*self.CHIPS_PER_MODULE_IN_LAYER[3],
                                         self.MODULES_PER_STAVE_IN_LAYER[4]*self.CHIPS_PER_MODULE_IN_LAYER[4],
                                         self.MODULES_PER_STAVE_IN_LAYER[5]*self.CHIPS_PER_MODULE_IN_LAYER[5],
                                         self.MODULES_PER_STAVE_IN_LAYER[6]*self.CHIPS_PER_MODULE_IN_LAYER[6])

        self.CHIPS_PER_LAYER = (self.STAVES_PER_LAYER[0]*self.CHIPS_PER_IB_STAVE,
                                self.STAVES_PER_LAYER[1]*self.CHIPS_PER_IB_STAVE,
                                self.STAVES_PER_LAYER[2]*self.CHIPS_PER_IB_STAVE,
                                self.STAVES_PER_LAYER[3]*self.MODULES_PER_MB_STAVE*self.CHIPS_PER_FULL_MODULE,
                                self.STAVES_PER_LAYER[4]*self.MODULES_PER_MB_STAVE*self.CHIPS_PER_FULL_MODULE,
                                self.STAVES_PER_LAYER[5]*self.MODULES_PER_OB_STAVE*self.CHIPS_PER_FULL_MODULE,
                                self.STAVES_PER_LAYER[6]*self.MODULES_PER_OB_STAVE*self.CHIPS_PER_FULL_MODULE)

        # Number of chips "before" a specific layer
        self.CUMULATIVE_CHIP_COUNT_AT_LAYER = (0
                                               ,
                                               self.CHIPS_PER_LAYER[0]
                                               ,
                                               self.CHIPS_PER_LAYER[0] +
                                               self.CHIPS_PER_LAYER[1]
                                               ,
                                               self.CHIPS_PER_LAYER[0] +
                                               self.CHIPS_PER_LAYER[1] +
                                               self.CHIPS_PER_LAYER[2]
                                               ,
                                               self.CHIPS_PER_LAYER[0] +
                                               self.CHIPS_PER_LAYER[1] +
                                               self.CHIPS_PER_LAYER[2] +
                                               self.CHIPS_PER_LAYER[3]
                                               ,
                                               self.CHIPS_PER_LAYER[0] +
                                               self.CHIPS_PER_LAYER[1] +
                                               self.CHIPS_PER_LAYER[2] +
                                               self.CHIPS_PER_LAYER[3] +
                                               self.CHIPS_PER_LAYER[4]
                                               ,
                                               self.CHIPS_PER_LAYER[0] +
                                               self.CHIPS_PER_LAYER[1] +
                                               self.CHIPS_PER_LAYER[2] +
                                               self.CHIPS_PER_LAYER[3] +
                                               self.CHIPS_PER_LAYER[4] +
                                               self.CHIPS_PER_LAYER[5]
                                               )

        self.CHIP_COUNT_TOTAL = sum(self.CHIPS_PER_LAYER)

        self.DATA_LINKS_PER_LAYER = (self.STAVES_PER_LAYER[0]*self.DATA_LINKS_PER_IB_STAVE,
                                     self.STAVES_PER_LAYER[1]*self.DATA_LINKS_PER_IB_STAVE,
                                     self.STAVES_PER_LAYER[2]*self.DATA_LINKS_PER_IB_STAVE,
                                     self.STAVES_PER_LAYER[3]*self.MODULES_PER_MB_STAVE*self.DATA_LINKS_PER_FULL_MODULE,
                                     self.STAVES_PER_LAYER[4]*self.MODULES_PER_MB_STAVE*self.DATA_LINKS_PER_FULL_MODULE,
                                     self.STAVES_PER_LAYER[5]*self.MODULES_PER_OB_STAVE*self.DATA_LINKS_PER_FULL_MODULE,
                                     self.STAVES_PER_LAYER[6]*self.MODULES_PER_OB_STAVE*self.DATA_LINKS_PER_FULL_MODULE)


        self.CTRL_LINKS_PER_LAYER = (self.STAVES_PER_LAYER[0]*self.CTRL_LINKS_PER_IB_STAVE,
                                     self.STAVES_PER_LAYER[1]*self.CTRL_LINKS_PER_IB_STAVE,
                                     self.STAVES_PER_LAYER[2]*self.CTRL_LINKS_PER_IB_STAVE,
                                     self.STAVES_PER_LAYER[3]*self.MODULES_PER_MB_STAVE*self.CTRL_LINKS_PER_FULL_MODULE,
                                     self.STAVES_PER_LAYER[4]*self.MODULES_PER_MB_STAVE*self.CTRL_LINKS_PER_FULL_MODULE,
                                     self.STAVES_PER_LAYER[5]*self.MODULES_PER_OB_STAVE*self.CTRL_LINKS_PER_FULL_MODULE,
                                     self.STAVES_PER_LAYER[6]*self.MODULES_PER_OB_STAVE*self.CTRL_LINKS_PER_FULL_MODULE)

    def position_to_global_chip_id(self, position: dict):
        chip_id = self.CUMULATIVE_CHIP_COUNT_AT_LAYER[position['layer_id']]
        chip_id += self.CHIPS_PER_STAVE_IN_LAYER[position['layer_id']] * position['stave_id']

        chip_id += position['sub_stave_id'] * self.MODULES_PER_SUB_STAVE_IN_LAYER[position['layer_id']] * self.CHIPS_PER_MODULE_IN_LAYER[position['layer_id']]

        chip_id += position['module_id'] * self.CHIPS_PER_MODULE_IN_LAYER[position['layer_id']]

        chip_id += position['module_chip_id']

        return chip_id

    def global_chip_id_to_position(self, global_chip_id: int):
        layer_id = 0
        sub_stave_id = 0

        while layer_id < self.N_LAYERS-1:
            if global_chip_id < self.CUMULATIVE_CHIP_COUNT_AT_LAYER[layer_id+1]:
                break
            else:
                layer_id += 1

        chip_num_in_layer = global_chip_id

        if layer_id > 0:
            chip_num_in_layer -= self.CUMULATIVE_CHIP_COUNT_AT_LAYER[layer_id]

        stave_id = int(chip_num_in_layer / self.CHIPS_PER_STAVE_IN_LAYER[layer_id])
        chip_num_in_stave = chip_num_in_layer % self.CHIPS_PER_STAVE_IN_LAYER[layer_id]

        module_id = int(chip_num_in_stave / self.CHIPS_PER_MODULE_IN_LAYER[layer_id])
        chip_num_in_module = chip_num_in_stave % self.CHIPS_PER_MODULE_IN_LAYER[layer_id]

        # Middle/outer barrel stave? Calculate sub stave id
        if layer_id > 2:
            sub_stave_id = int(module_id / self.MODULES_PER_SUB_STAVE_IN_LAYER[layer_id])
            module_id = module_id % self.MODULES_PER_SUB_STAVE_IN_LAYER[layer_id]

        position = {'layer_id': layer_id,
                    'stave_id': stave_id,
                    'sub_stave_id': sub_stave_id,
                    'module_id': module_id,
                    'module_chip_id': chip_num_in_module}

        return position

    def data_link_id_to_sub_stave_and_module_id(self, data_link_id, layer):
        if layer < 3:
            module_id = 0
            sub_stave_id = 0
        else:
            links_per_stave = int(self.DATA_LINKS_PER_LAYER[layer] / self.STAVES_PER_LAYER[layer])
            links_per_sub_stave = int(links_per_stave / self.SUB_STAVES_PER_STAVE[layer])

            sub_stave_id = int(data_link_id / links_per_sub_stave)

            link_in_sub_stave = data_link_id % links_per_sub_stave

            module_id = int(link_in_sub_stave / self.DATA_LINKS_PER_FULL_MODULE)

        return {'sub_stave_id': sub_stave_id, 'module_id': module_id}


if __name__ == '__main__':
    its = ITS()

    data = list()

    for chip_id in range(0, its.CHIP_COUNT_TOTAL):
        chip_position = its.global_chip_id_to_position(chip_id)
        data.append([chip_id,
                     chip_position['layer_id'],
                     chip_position['stave_id'],
                     chip_position['sub_stave_id'],
                     chip_position['module_id'],
                     chip_position['chip_num_in_module']])

    df = pd.DataFrame(data, columns=['global_chip_id', 'layer_id', 'stave_id', 'sub_stave_id', 'module_id', 'module_chip_id'])

    # Check that we calculate the same global chip id when going in the reverse direction
    for index, row in df.iterrows():
        global_chip_id = its.position_to_global_chip_id({'layer_id': row['layer_id'],
                                                         'stave_id': row['stave_id'],
                                                         'sub_stave_id': row['sub_stave_id'],
                                                         'module_id': row['module_id'],
                                                         'module_chip_id': row['module_chip_id']})

        assert global_chip_id == row['global_chip_id'], 'Got chip ID ' + str(global_chip_id) + ' expected ' + row['global_chip_id']

    df.to_csv('its_positions.csv', index=None, sep=';')
    print(df)
