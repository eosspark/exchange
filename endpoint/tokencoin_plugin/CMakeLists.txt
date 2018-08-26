file(GLOB HEADERS "include/eosio/tokencoin_plugin/*.hpp")
add_library( tokencoin_plugin
             tokencoin_plugin.cpp
             ${HEADERS} )
             
target_link_libraries( tokencoin_plugin history_plugin chain_plugin eosio_chain appbase )
target_include_directories( tokencoin_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
