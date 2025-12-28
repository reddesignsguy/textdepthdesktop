# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles/appTextDepthOG_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/appTextDepthOG_autogen.dir/ParseCache.txt"
  "appTextDepthOG_autogen"
  )
endif()
