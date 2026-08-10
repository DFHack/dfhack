dfhack_plugin(smooth-movement smooth-movement.cpp visual_animation.cpp visual_renderer.cpp
              LINK_LIBRARIES lua)
target_include_directories(smooth-movement PRIVATE ${SDL2_INCLUDE_DIRS})
dfhack_test(smooth-movement-test "test_visual_animation_manager.cpp;visual_animation.cpp")
