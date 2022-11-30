idf_component_register(
    SRCS "flowmeter_main.c" 
         "INA219/INA219.c" "INA219/INA219.h" "INA219/INA219_consts.h" 
         "flow_estimation/circular_buffer.h" "flow_estimation/messages.h" "flow_estimation/messages.c"
         "transmit/transmit.h" "transmit/transmit.c"
    INCLUDE_DIRS "")

target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
