set(MODEL_SOURCES
  ${MODEL_SOURCES}

  source/bench/Alpide/AlpideBase/AlpideBase.cpp
  source/bench/Alpide/AlpideDataflow/AlpideDataflow.cpp
  source/bench/Alpide/AlpideDataflow/Alpide.cpp
  source/bench/Alpide/AlpideDataflow/PixelDoubleColumn.cpp
  source/bench/Alpide/AlpideDataflow/PixelMatrix.cpp
  source/bench/Alpide/AlpideDataflow/RegionReadoutUnit.cpp
  source/bench/Alpide/AlpideDataflow/TopReadoutUnit.cpp
  source/bench/Alpide/AlpideSimple/AlpideDataGenerator.cpp
  source/bench/Alpide/AlpideSimple/AlpideSimple.cpp

  )

add_sources(${MODEL_SOURCES})
