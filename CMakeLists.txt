set(MODEL_SOURCES
  ${MODEL_SOURCES}

  source/bench/alpide/Alpide.cpp
  source/bench/alpide/AlpideDataParser.cpp
  source/bench/alpide/PixelDoubleColumn.cpp
  source/bench/alpide/PixelMatrix.cpp
  source/bench/alpide/RegionReadoutUnit.cpp
  source/bench/alpide/TopReadoutUnit.cpp

  )

add_sources(${MODEL_SOURCES})
