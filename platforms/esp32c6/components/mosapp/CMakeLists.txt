separate_arguments(app_srcs NATIVE_COMMAND "$ENV{APP_SOURCES}")
idf_component_register(
  SRCS ${app_srcs}
  REQUIRES main
)
