set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/examples/system/console/components)
set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/components)
idf_component_register(SRCS "cmd_ledtools.c"
                    INCLUDE_DIRS "."
                                 "include"
                                 "$ENV{IDF_PATH}/components/console")