/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosio/tokencoin_plugin/tokencoin_plugin.hpp>
#include <eosio/http_plugin/http_plugin.hpp>

#include <appbase/application.hpp>
#include <eosio/chain/controller.hpp>

namespace eosio {
   using eosio::chain::controller;
   using std::unique_ptr;
   using namespace appbase;

   class tokencoin_api_plugin : public plugin<tokencoin_api_plugin> {
      public:
        APPBASE_PLUGIN_REQUIRES((http_plugin)(tokencoin_plugin))

        tokencoin_api_plugin();
        virtual ~tokencoin_api_plugin();

        virtual void set_program_options(options_description&, options_description&) override;

        void plugin_initialize(const variables_map&);
        void plugin_startup();
        void plugin_shutdown();
   };

}
