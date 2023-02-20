cmake_minimum_required(VERSION 3.5)
set(PYTHON_DEPS_CHECKED 1)  # Save some build time
set(EXTRA_CFLAGS $ENV{EXTRA_CFLAGS})
set(EXTRA_CPPFLAGS $ENV{EXTRA_CPPFLAGS})
set(EXTRA_CXXFLAGS $ENV{EXTRA_CXXFLAGS})

# Add binary libs.
separate_arguments(app_bin_libs NATIVE_COMMAND "$ENV{APP_BIN_LIBS}")
foreach(bin_lib ${app_bin_libs})
  get_filename_component(target_name ${bin_lib} NAME_WLE)
  add_prebuilt_library(${target_name} ${bin_lib})
  target_link_libraries(${CMAKE_PROJECT_NAME}.elf ${target_name})
endforeach()

# ESP-IDF passes component libs multiple times to the linker.
# This looks like a bug but is not a problem unless they are --whole-archive's.
# Which is what we want to do, since that is the only way to make sure weak
# functions are overridden by non-weak ones, which we definitely want to happen.
# We wrap the link command with a script that dedupes the static libs,
# puts them in a group and wraps with --whole-archive.
set(CMAKE_CXX_LINK_EXECUTABLE "$ENV{MGOS_PATH}/tools/link.py <CMAKE_CXX_COMPILER> <FLAGS> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
