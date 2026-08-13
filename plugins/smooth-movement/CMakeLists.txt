dfhack_plugin(smooth-movement smooth-movement.cpp LINK_LIBRARIES lua)
target_include_directories(smooth-movement PRIVATE ${SDL2_INCLUDE_DIRS})
dfhack_test(smooth-movement-test test_visual_animation_manager.cpp)
