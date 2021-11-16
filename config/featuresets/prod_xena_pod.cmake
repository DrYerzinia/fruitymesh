set(PLATFORM NRF52833)
set(FAIL_ON_SIZE_TOO_BIG 0)

# add the SDK driver for SPI
include(${PROJECT_SOURCE_DIR}/config/featuresets/CMakeFragments/AddSdkTwi.cmake)
include(${PROJECT_SOURCE_DIR}/config/featuresets/CMakeFragments/AddSdkSpi.cmake)
