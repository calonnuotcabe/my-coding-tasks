set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_AR arm-none-eabi-ar)
set(CMAKE_RANLIB arm-none-eabi-ranlib)

set(BB_ARM_FLAGS "-mcpu=cortex-r5 -marm -mfloat-abi=soft")
set(CMAKE_C_FLAGS_INIT "${BB_ARM_FLAGS}")
set(CMAKE_ASM_FLAGS_INIT "${BB_ARM_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${BB_ARM_FLAGS}")

