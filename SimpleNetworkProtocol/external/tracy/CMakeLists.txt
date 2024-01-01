
add_library(Tracy
    tracy/public/TracyClient.cpp
)
set_property(TARGET Tracy PROPERTY SYSTEM TRUE)
target_include_directories(Tracy SYSTEM INTERFACE tracy/public/tracy)
if (WIN32)
  target_compile_options(Tracy PRIVATE "/W0")
else ()
  target_compile_options(Tracy PRIVATE "-w")
endif ()
if (ENABLE_TRACY)
  target_compile_definitions(Tracy PUBLIC "TRACY_ENABLE")
  target_compile_definitions(Tracy PUBLIC "TRACY_NO_EXIT=1")
endif ()
# set_property(TARGET Tracy PROPERTY FOLDER "External")
