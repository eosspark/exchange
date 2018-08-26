file(GLOB HEADERS "include/eosio/tokencoin_api_plugin/*.hpp")
add_library( tokencoin_api_plugin
             tokencoin_api_plugin.cpp
             ${HEADERS} )

target_link_libraries( tokencoin_api_plugin tokencoin_plugin chain_plugin http_plugin appbase )
target_include_directories( tokencoin_api_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
