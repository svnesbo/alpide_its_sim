#include "AlpideDataGenerator.hpp"

int main() {

  int const NR_EVENTS = 1000;

  AlpideDataGenerator m_datagen(100, 1, 1,true);

  for(int i = 0; i < NR_EVENTS; ++i) {
    m_datagen.clearData();
    m_datagen.generateChipHit(0x00,0,true);
  }

  m_datagen.clearData();

  return 0;
}
