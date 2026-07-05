separate_arguments(gen_srcs NATIVE_COMMAND "$ENV{GEN_SOURCES}")
separate_arguments(mgos_srcs NATIVE_COMMAND "$ENV{MGOS_SOURCES}")
idf_component_register(
  INCLUDE_DIRS "."
  SRCS ${app_srcs} ${gen_srcs} ${mgos_srcs}
)
# In ffi exports file we use fake signatures: void func(void), and it conflicts
# with the builtin functions like fopen, etc.
set_source_files_properties(${FFI_EXPORTS_C} PROPERTIES COMPILE_FLAGS -fno-builtin)
