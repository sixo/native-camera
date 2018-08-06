# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

file( GLOB app_src_files
     "src/main/cpp/*.cpp" )

add_library( native-lib SHARED ${app_src_files} )

find_library( log-lib log )
find_library( camera-lib camera2ndk )
find_library( media-lib mediandk )
find_library( android-lib android )
find_library( gl-lib GLESv2 )
target_link_libraries( native-lib  ${log-lib} ${camera-lib} ${media-lib} ${android-lib} ${gl-lib})