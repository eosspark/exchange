/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <appbase/application.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/chain/plugin_interface.hpp>
//#include <eosiolib/asset.hpp>

namespace fc { class variant; }

namespace eosio {
   using chain::transaction_id_type;
   using std::shared_ptr;
   using namespace appbase;
   using chain::name;
   using fc::optional;
   using chain::uint128_t;
   using chain::authority;
   using chain::asset;
   using chain::symbol;

   using get_info_results         =  chain_apis::read_only::get_info_results;
   using abi_json_to_bin_result   =  chain_apis::read_only::abi_json_to_bin_result;
   using get_required_keys_result = chain_apis::read_only::get_required_keys_result;
   using push_transaction_results = chain_apis::read_write::push_transaction_results;

   typedef double real_type;

   typedef shared_ptr<class tokencoin_plugin_impl> tokencoin_ptr;
   typedef shared_ptr<const class tokencoin_plugin_impl> tokencoin_const_ptr;

namespace tokencoin_apis {

   struct permission {
   name              perm_name;
   name              parent;
   authority         required_auth;
   };

class read_only {
   public:
      const controller& db;
      const fc::microseconds abi_serializer_max_time;
      tokencoin_const_ptr tokencoin;

   public:
      read_only(const controller& db, const fc::microseconds& abi_serializer_max_time, tokencoin_const_ptr&& tokencoin)
         : db(db), abi_serializer_max_time(abi_serializer_max_time), tokencoin(tokencoin) {}

      struct get_required_keys2_params {
        fc::variant transaction;
        flat_set<public_key_type> available_keys;
      };
      struct get_required_keys2_result {
        int                      code;
        string                   message;
        get_required_keys_result data;
      };
      get_required_keys2_result get_required_keys2( const get_required_keys2_params& params)const;

      struct abi_json_to_bin2_params {
        name         code;
        name         action;
        fc::variant  args;
      };
      struct abi_json_to_bin2_result {
        int                    code;
        string                 message;
        abi_json_to_bin_result data;
      };
      abi_json_to_bin2_result abi_json_to_bin2( const abi_json_to_bin2_params& params )const;

      using get_info2_params = chain_apis::empty;
      struct get_info2_results {
        int              code;
        string           message;
        get_info_results data;
      };
      get_info2_results get_info2(const get_info2_params& params) const;

      struct pair_data_bean_buy {
            account_name from;
            asset        quant;
            asset        symbol;
            account_name feeto;

            pair_data_bean_buy() = default;
            pair_data_bean_buy(const account_name &from, 
                            const asset &quant, 
                            const asset &symbol, 
                            const account_name &feeto) : from(from),
                                                 quant(quant),
                                                 symbol(symbol),
                                                 feeto(feeto) {};
            static name get_account() {
                return N(cactus.token);
            }

            static name get_name() {
                return N(buy);
            }
      };
      struct pair_data_bean_sell {
            account_name from;
            asset        quant;
            asset        symbol;
            account_name feeto;

            pair_data_bean_sell() = default;
            pair_data_bean_sell(const account_name &from, 
                            const asset &quant, 
                            const asset &symbol, 
                            const account_name &feeto) : from(from),
                                                 quant(quant),
                                                 symbol(symbol),
                                                 feeto(feeto) {};
            static name get_account() {
                return N(cactus.token);
            }

            static name get_name() {
                return N(sell);
            }
      };
      struct pair_data_bean {
            account_name from;
            asset        quant;
            asset        symbol;
            account_name feeto;
            string       pair;
            pair_data_bean& operator=( pair_data_bean_sell v ){
               this->from   = v.from;
               this->quant  = v.quant;
               this->symbol = v.symbol;
               this->feeto  = v.feeto;
               this->pair   = v.quant.symbol_name() + v.symbol.symbol_name();
               return *this;
            }
            pair_data_bean& operator=( pair_data_bean_buy v ){
               this->from   = v.from;
               this->quant  = v.quant;
               this->symbol = v.symbol;
               this->feeto  = v.feeto;
               this->pair   =  v.symbol.symbol_name() + v.quant.symbol_name();
               return *this;
            }

      };
      struct pair_action_bean {
         transaction_id_type          transcation_id;
         uint32_t                     block_num;
         chain::block_timestamp_type  block_time;
         int32_t                      seq_num;
         pair_data_bean               data;
      };
      struct get_pair_actions_data {
         vector<pair_action_bean>    actions;
         uint32_t                    last_irreversible_block;
      }; 

      struct get_pair_transactions_results{
         int                  code;
         string               message;
         get_pair_actions_data       data;
      };

      struct get_pair_transactions_params{
         name account_name;
         optional<int32_t> num_seq;
         optional<int32_t> skip_seq;
      };      
      get_pair_transactions_results get_pair_transactions( const get_pair_transactions_params& params )const;

      struct connector {
         asset balance ;
         double weight ;
         //EOSLIB_SERIALIZE( connector, (balance)(weight) )
      };
      struct exchange_state {
         asset    supply;
         connector base;
         connector quote;
         uint64_t  fee_amount;//fee = 1/free_amount

         //uint64_t primary_key()const { return supply.symbol_type.name(); }
         asset convert_to_exchange( connector& c, asset in ); 
         asset convert_from_exchange( connector& c, asset in );
         asset convert( asset from, symbol to );
      };


      struct pair {
         asset supply;
         connector base;
         connector quote;
         uint64_t fee_amount;
         double token_price_pair;
         double token_price_usd;
         double token_price_cny;
         //EOSLIB_SERIALIZE( pair, (supply)(base)(quote)(fee_amount)(token_price_usd)(token_price_cny) )
      };
      struct get_pairs_data {
         vector<pair> pairs;
      };
      struct get_pairs_results{
         int                  code;
         string               message;
         get_pairs_data       data;
      };
      struct get_pairs_params{
         vector<string> pairs;
      };
      get_pairs_results get_pairs( const get_pairs_params& params )const;

      //for tokencoin
      struct get_account_app_data {
         asset          staked_balance;
         name           account_name;
         fc::time_point last_unstaking_time;
         asset          unstaking_balance;
         asset          eos_balance;

         vector<permission>         permissions;  
      };
      struct get_account_app_results {
         int                  code;
         string               message;
         get_account_app_data data;
      };
      struct get_account_app_params {
         name account_name;
      };
      get_account_app_results get_account_app( const get_account_app_params& params )const;

      ////////////////////
      struct asset_data {
        string contract;
        asset  asset;
      };
      struct get_account_asset_data {
         name                account_name;
         string              account_icon;
         vector<asset_data>  account_assets;
      };
      struct get_account_asset_results {
         int                    code;
         string                 message;
         get_account_asset_data data;
      }; 
      struct get_account_asset_params {
         name account_name;
      };
      get_account_asset_results get_account_asset(const get_account_asset_params& params )const;

      ////////////////////
      struct get_sparklines_data {
         string         sparkline_token_png;
      };
      struct get_sparklines_results {
         int                    code;
         string                 message;
         get_sparklines_data    data;
      }; 
      struct get_sparklines_params {
         string token_symbol;
      };
      get_sparklines_results get_sparklines(const get_sparklines_params& params )const;

      struct data_bean {
            account_name from;
            account_name to;
            asset        quantity;
            string       memo;

            data_bean() = default;
            data_bean(const account_name &from, 
                            const account_name &to, 
                            const asset &quantity, 
                            const string &memo) : from(from),
                                                 to(to),
                                                 quantity(quantity),
                                                 memo(memo) {};
            static name get_account() {
                return N(eosio.token);
            }

            static name get_name() {
                return N(transfer);
            }
      };
      struct action_bean {
         transaction_id_type          transcation_id;
         uint32_t                     block_num;
         chain::block_timestamp_type  block_time;
         int32_t                      seq_num;
         data_bean                    data;
      };
      struct get_actions_data {
         vector<action_bean>    actions;
         uint32_t               last_irreversible_block;
      };   
      struct get_transactions_results {
         int                    code;
         string                 message;
         get_actions_data       data;
      }; 
      struct get_transactions_params{
         name account_name;
         optional<int32_t> num_seq;
         optional<int32_t> skip_seq;
         string contract;
      };
      get_transactions_results get_transactions(const get_transactions_params& params )const;
      //////////////////////

};

class read_write {
public:
   controller& db;
   const fc::microseconds abi_serializer_max_time;
public:
   read_write(controller& db, const fc::microseconds& abi_serializer_max_time)
         : db(db), abi_serializer_max_time(abi_serializer_max_time) {}

   using push_transaction2_params = fc::variant_object;
   struct push_transaction2_results {
      int                      code;
      string                   message;
      push_transaction_results data;
   }; 
   void push_transaction2(const push_transaction2_params& params, chain::plugin_interface::next_function<push_transaction2_results> next);

};


} // namespace tokencoin_apis


/**
 *  This plugin tracks all actions and keys associated with a set of configured accounts. It enables
 *  wallets to paginate queries for tokencoin.  
 *
 *  An action will be included in the account's tokencoin if any of the following:
 *     - receiver
 *     - any account named in auth list
 *
 *  A key will be linked to an account if the key is referneced in authorities of updateauth or newaccount 
 */
class tokencoin_plugin : public plugin<tokencoin_plugin> {
   public:
      APPBASE_PLUGIN_REQUIRES((chain_plugin))

      tokencoin_plugin();
      virtual ~tokencoin_plugin();

      virtual void set_program_options(options_description& cli, options_description& cfg) override;

      void plugin_initialize(const variables_map& options);
      void plugin_startup();
      void plugin_shutdown();

      // Only call this after plugin_startup()!
     controller& chain();
     const controller& chain() const;
     fc::microseconds get_abi_serializer_max_time() const;

      tokencoin_apis::read_only  get_read_only_api()const { return tokencoin_apis::read_only(chain(), get_abi_serializer_max_time(), tokencoin_const_ptr(my)); }
      tokencoin_apis::read_write get_read_write_api();

   private:
      tokencoin_ptr my;
};

} /// namespace eosio

/*
FC_REFLECT(eosio::tokencoin_apis::read_only::get_transaction_params, (transaction_id) )
FC_REFLECT(eosio::tokencoin_apis::read_only::get_transaction_results, (transaction_id)(transaction) )
FC_REFLECT(eosio::tokencoin_apis::read_only::get_transactions_params, (account_name)(skip_seq)(num_seq) )
FC_REFLECT(eosio::tokencoin_apis::read_only::ordered_transaction_results, (seq_num)(transaction_id)(transaction) )
FC_REFLECT(eosio::tokencoin_apis::read_only::get_transactions_results, (transactions)(time_limit_exceeded_error) )
*/
// FC_REFLECT(eosio::tokencoin_apis::read_only::get_key_accounts_params, (public_key) )
// FC_REFLECT(eosio::tokencoin_apis::read_only::get_key_accounts_results, (account_names) )
// FC_REFLECT(eosio::tokencoin_apis::read_only::get_controlled_accounts_params, (controlling_account) )
// FC_REFLECT(eosio::tokencoin_apis::read_only::get_controlled_accounts_results, (controlled_accounts) )
// FC_REFLECT(eosio::tokencoin_apis::empty, )
// FC_REFLECT(eosio::tokencoin_apis::read_only::get_info_data,
// (server_version)(chain_id)(head_block_num)(last_irreversible_block_num)(last_irreversible_block_id)(head_block_id)(head_block_time)(head_block_producer)(virtual_block_cpu_limit)(virtual_block_net_limit)(block_cpu_limit)(block_net_limit) )
// FC_REFLECT( eosio::tokencoin_apis::read_only::get_info2_results,(code)(message)(data))

FC_REFLECT( eosio::tokencoin_apis::permission, (perm_name)(parent)(required_auth) )

FC_REFLECT( eosio::tokencoin_apis::read_only::get_account_app_results,(code)(message)(data))
FC_REFLECT( eosio::tokencoin_apis::read_only::get_account_app_data,(staked_balance)(account_name)(last_unstaking_time)(unstaking_balance)(eos_balance)(permissions))
FC_REFLECT( eosio::tokencoin_apis::read_only::get_account_app_params, (account_name) )

FC_REFLECT( eosio::tokencoin_apis::read_only::get_account_asset_results,(code)(message)(data))
FC_REFLECT( eosio::tokencoin_apis::read_only::get_account_asset_data,(account_name)(account_icon)(account_assets))
FC_REFLECT( eosio::tokencoin_apis::read_only::get_account_asset_params, (account_name) )
FC_REFLECT( eosio::tokencoin_apis::read_only::asset_data, (contract)(asset) )

FC_REFLECT( eosio::tokencoin_apis::read_only::get_sparklines_results,(code)(message)(data))
FC_REFLECT( eosio::tokencoin_apis::read_only::get_sparklines_data,(sparkline_token_png))
FC_REFLECT( eosio::tokencoin_apis::read_only::get_sparklines_params, (token_symbol) )

FC_REFLECT( eosio::tokencoin_apis::read_only::data_bean,(from)(to)(quantity)(memo))
FC_REFLECT( eosio::tokencoin_apis::read_only::action_bean,(transcation_id)(block_num)(block_time)(data)(seq_num))
FC_REFLECT( eosio::tokencoin_apis::read_only::get_actions_data,(actions)(last_irreversible_block))
FC_REFLECT( eosio::tokencoin_apis::read_only::get_transactions_results,(code)(message)(data))
FC_REFLECT( eosio::tokencoin_apis::read_only::get_transactions_params, (account_name) (num_seq) (skip_seq) (contract) )

FC_REFLECT( eosio::tokencoin_apis::read_only::exchange_state,(supply)(base)(quote)(fee_amount))
FC_REFLECT( eosio::tokencoin_apis::read_only::connector,(balance)(weight))
FC_REFLECT( eosio::tokencoin_apis::read_only::pair,(supply)(base)(quote)(fee_amount)(token_price_pair)(token_price_usd)(token_price_cny))
FC_REFLECT( eosio::tokencoin_apis::read_only::get_pairs_data,(pairs))
FC_REFLECT( eosio::tokencoin_apis::read_only::get_pairs_results,(code)(message)(data))
FC_REFLECT( eosio::tokencoin_apis::read_only::get_pairs_params, (pairs))

FC_REFLECT( eosio::tokencoin_apis::read_only::pair_data_bean_buy,(from)(quant)(symbol)(feeto))
FC_REFLECT( eosio::tokencoin_apis::read_only::pair_data_bean_sell,(from)(quant)(symbol)(feeto))
FC_REFLECT( eosio::tokencoin_apis::read_only::pair_data_bean,(from)(quant)(symbol)(feeto)(pair))
FC_REFLECT( eosio::tokencoin_apis::read_only::pair_action_bean,(transcation_id)(block_num)(block_time)(seq_num)(data))
FC_REFLECT( eosio::tokencoin_apis::read_only::get_pair_actions_data,(actions)(last_irreversible_block))
FC_REFLECT( eosio::tokencoin_apis::read_only::get_pair_transactions_results,(code)(message)(data))
FC_REFLECT( eosio::tokencoin_apis::read_only::get_pair_transactions_params,(account_name)(num_seq)(skip_seq))

FC_REFLECT( eosio::tokencoin_apis::read_only::abi_json_to_bin2_params, (code)(action)(args) )
FC_REFLECT( eosio::tokencoin_apis::read_only::abi_json_to_bin2_result, (code)(message)(data) )
FC_REFLECT( eosio::tokencoin_apis::read_only::get_required_keys2_params, (transaction)(available_keys) )
FC_REFLECT( eosio::tokencoin_apis::read_only::get_required_keys2_result,(code)(message)(data))
FC_REFLECT( eosio::tokencoin_apis::read_only::get_info2_results,(code)(message)(data))
FC_REFLECT( eosio::tokencoin_apis::read_write::push_transaction2_results,(code)(message)(data))
