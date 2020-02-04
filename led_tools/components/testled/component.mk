include($ENV{IDF_PATH}/tools/cmake/project.cmake)
EXTRA_COMPONENT_DIRS = $(IDF_PATH)/examples/system/console/components
EXTRA_COMPONENT_DIRS = $(IDF_PATH)/components/console

include $(IDF_PATH)/make/project.mk