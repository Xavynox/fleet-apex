# Toolchain file for Android
set(CMAKE_SYSTEM_NAME Android)
set(CMAKE_SYSTEM_PROCESSOR arm64)

# Toolchain files
set(CMAKE_C_COMPILER   /data/user/0/com.androidharness.app/files/linux/bin/clang++)
set(CMAKE_CXX_COMPILER /data/user/0/com.androidharness.app/files/linux/bin/clang++)

# Android-specific flags
set(CMAKE_SYSTEM_NAME Android)
set(CMAKE_SYSTEM_PROCESSOR arm64)

# Set the C and C++ compilers
set(CMAKE_C_COMPILER   /data/user/0/com.androidharness.app/files/linux/bin/clang++)
set(CMAKE_CXX_COMPILER /data/user/0/com.androidharness.app/files/linux/bin/clang++)

# Android-specific flags
set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -std=c++17 -fPIC")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++17 -fPIC")
