
find_package(OpenCV)

if(OpenCV_FOUND)

  message(STATUS "Opencv include dir? ${OpenCV_INCLUDE_DIRS}" )
  
  link_directories( ${Boost_LIBRARY_DIRS} )
  include_directories( ${Boost_INCLUDE_DIRS} ${OpenCV_INCLUDE_DIRS} )

  add_executable(dv_hax  main.cc )
  target_link_libraries(dv_hax  dvgrab  ${OpenCV_LIBS} )
  
  add_executable(test_dv_client  dv_client.cpp )
  target_link_libraries(test_dv_client  dvgrab ${Boost_LIBRARIES}  ${OpenCV_LIBS})

endif()
