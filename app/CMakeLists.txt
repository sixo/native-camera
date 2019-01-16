cmake_minimum_required(VERSION 3.4.1)


file( GLOB app_src_files
     "src/main/cpp/*.cpp" )

add_library( native-lib SHARED ${app_src_files} )

# Enable this if you want to use AImageReader
#target_compile_definitions( native-lib PRIVATE WITH_IMAGE_READER=1 )

find_library( log-lib log )
find_library( camera-lib camera2ndk )
find_library( media-lib mediandk )
find_library( android-lib android )
find_library( gl-lib GLESv2 )
target_link_libraries( native-lib  ${log-lib} ${camera-lib} ${media-lib} ${android-lib} ${gl-lib})