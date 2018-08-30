#include <eosio/tokencoin_plugin/tokencoin_plugin.hpp>
#include <eosio/history_plugin/history_plugin.hpp>
//#include <eosio/http_plugin/http_plugin.hpp>
#include <eosio/chain/controller.hpp>
#include <eosio/chain/trace.hpp>
//#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/chain/authorization_manager.hpp>
#include <boost/algorithm/string.hpp>
// #include <eosio/utilities/common.hpp>
// #include <eosio/utilities/key_conversion.hpp>


namespace eosio { 
   using namespace chain;
   using namespace chain::config;
   using namespace chain_apis;
   using namespace eosio::chain::plugin_interface;

   //using boost::signals2::scoped_connection;

   static appbase::abstract_plugin& _tokencoin_plugin = app().register_plugin<tokencoin_plugin>();

   #define CATCH_AND_CALL(NEXT)\
      catch ( const fc::exception& err ) {\
         NEXT(err.dynamic_copy_exception());\
      } catch ( const std::exception& e ) {\
         fc::exception fce( \
            FC_LOG_MESSAGE( warn, "rethrow ${what}: ", ("what",e.what())),\
            fc::std_exception_code,\
            BOOST_CORE_TYPEID(e).name(),\
            e.what() ) ;\
         NEXT(fce.dynamic_copy_exception());\
      } catch( ... ) {\
         fc::unhandled_exception e(\
            FC_LOG_MESSAGE(warn, "rethrow"),\
            std::current_exception());\
         NEXT(e.dynamic_copy_exception());\
      }
   

   class tokencoin_plugin_impl {
      public:
         chain_plugin*          chain_plug = nullptr;
         fc::microseconds       abi_serializer_max_time_ms;
   };

   tokencoin_plugin::tokencoin_plugin()
   :my(std::make_shared<tokencoin_plugin_impl>()) {}
   tokencoin_plugin::~tokencoin_plugin() {}
   void tokencoin_plugin::set_program_options(options_description& cli, options_description& cfg) {}
   void tokencoin_plugin::plugin_initialize(const variables_map& options) {
      my->chain_plug = app().find_plugin<chain_plugin>();
      my->abi_serializer_max_time_ms = fc::microseconds(config::default_abi_serializer_max_time_ms);
   }
   void tokencoin_plugin::plugin_startup() {}
   void tokencoin_plugin::plugin_shutdown() {}

   controller& tokencoin_plugin::chain() { return my->chain_plug->chain(); }
   const controller& tokencoin_plugin::chain() const { return my->chain_plug->chain(); }
   fc::microseconds tokencoin_plugin::get_abi_serializer_max_time() const {
      return my->abi_serializer_max_time_ms;
   }

   tokencoin_apis::read_write tokencoin_plugin::get_read_write_api() {
       return tokencoin_apis::read_write(chain(), get_abi_serializer_max_time());
   }

namespace tokencoin_apis { 
      template<typename Api>
      struct resolver_factory2 {
         static auto make(const Api* api, const fc::microseconds& max_serialization_time) {
            return [api, max_serialization_time](const account_name &name) -> optional<abi_serializer> {
               const auto* accnt = api->db.db().template find<account_object, by_name>(name);
               if (accnt != nullptr) {
                  abi_def abi;
                  if (abi_serializer::to_abi(accnt->abi, abi)) {
                     return abi_serializer(abi, max_serialization_time);
                  }
               }

               return optional<abi_serializer>();
            };
         }
      };

      template<typename Api>
      auto make_resolver2(const Api* api, const fc::microseconds& max_serialization_time) {
         return resolver_factory2<Api>::make(api, max_serialization_time);
      }

      read_only::get_info2_results read_only::get_info2(const read_only::get_info2_params& params) const {
         //const auto& db = tokencoin->chain_plug->chain();
         //const auto& rm = db.get_resource_limits_manager();

         get_info2_results result;
         result.code = 0;
         result.message = "ok";  

         auto ro_api = app().find_plugin<chain_plugin>()->get_read_only_api();
         result.data = ro_api.get_info( params );      

         return result;
      }

      read_only::abi_json_to_bin2_result read_only::abi_json_to_bin2( const read_only::abi_json_to_bin2_params& params )const try {
         abi_json_to_bin2_result result;
         result.code = 0;
         result.message = "ok"; 

         auto ro_api = app().find_plugin<chain_plugin>()->get_read_only_api();
         result.data = ro_api.abi_json_to_bin({params.code, params.action, params.args});          
         return result;
      } FC_RETHROW_EXCEPTIONS( warn, "code: ${code}, action: ${action}, args: ${args}",
                               ("code", params.code)( "action", params.action )( "args", params.args ))

      read_only::get_required_keys2_result read_only::get_required_keys2( const get_required_keys2_params& params )const {
         get_required_keys2_result result;
         result.code = 0;
         result.message = "ok";  

         auto ro_api = app().find_plugin<chain_plugin>()->get_read_only_api();
         result.data = ro_api.get_required_keys({params.transaction, params.available_keys});  

         //result.data.required_keys = required_keys_set;
         return result;
      }

      void read_write::push_transaction2(const read_write::push_transaction2_params& params,  chain::plugin_interface::next_function<read_write::push_transaction2_results> next) {


         // auto rw_api = app().find_plugin<chain_plugin>()->get_read_write_api();
         // result.data = ro_api.push_transaction(params , [this, next](const fc::static_variant<fc::exception_ptr, push_transaction_results>& result){
               // if (result.contains<fc::exception_ptr>()) {
               //    next(result.get<fc::exception_ptr>());
               // } else {
               //    next(read_write::push_transaction2_results{0,"ok", result});
               // } CATCH_AND_CALL(next);          
         // }); 
         
         try {
            auto pretty_input = std::make_shared<packed_transaction>();
            auto resolver = make_resolver2(this, abi_serializer_max_time);
            try {
               abi_serializer::from_variant(params, *pretty_input, resolver, abi_serializer_max_time);
            } EOS_RETHROW_EXCEPTIONS(chain::packed_transaction_type_exception, "Invalid packed transaction")

            app().get_method<incoming::methods::transaction_async>()(pretty_input, true, [this, next](const fc::static_variant<fc::exception_ptr, transaction_trace_ptr>& result) -> void{
               if (result.contains<fc::exception_ptr>()) {
                  next(result.get<fc::exception_ptr>());
               } else {
                  auto trx_trace_ptr = result.get<transaction_trace_ptr>();

                  try {
                     fc::variant pretty_output;
                     pretty_output = db.to_variant_with_abi(*trx_trace_ptr, abi_serializer_max_time);

                     chain::transaction_id_type id = trx_trace_ptr->id;
                     next(read_write::push_transaction2_results{0,"ok",{id, pretty_output}});
                  } CATCH_AND_CALL(next);
               }
            });


         } catch ( boost::interprocess::bad_alloc& ) {
            raise(SIGUSR1);
         } CATCH_AND_CALL(next);
      }


      read_only::get_pair_transactions_results read_only::get_pair_transactions( const get_pair_transactions_params& params )const{
         get_pair_transactions_results result;
         result.code = 0;
         result.message = "ok"; 

         auto ro_api = app().find_plugin<history_plugin>()->get_read_only_api();
         auto actions = ro_api.get_actions({params.account_name, params.num_seq, params.skip_seq});
         //auto actions = ro_api.get_actions({params.account_name, -1, -20});
         result.data.last_irreversible_block = actions.last_irreversible_block;
         
         //elog("actions:${actions}",("actions",actions));

         for(const auto& action: actions.actions ){
            //elog("action:${action}",("action",action));
            pair_action_bean action_b;
            action_b.block_num      = action.block_num;
            action_b.block_time     = action.block_time;
            action_b.seq_num        = action.account_action_seq;

            auto pretty_input = std::make_shared<action_trace>();
            auto resolver = make_resolver2(this,abi_serializer_max_time);
            try {
               abi_serializer::from_variant(action.action_trace, *pretty_input, resolver, abi_serializer_max_time);
            } EOS_RETHROW_EXCEPTIONS(chain::action_type_exception, "Invalid packed action_trace")
            //elog("pretty_input:${pretty_input}",("pretty_input",pretty_input));
            if(pretty_input->act.account == N(cactus.token) && pretty_input->act.name == N(buy)){
               action_b.transcation_id = pretty_input->trx_id;
               pair_data_bean_buy buy = pretty_input->act.data_as<pair_data_bean_buy>();
               action_b.data = buy;
               result.data.actions.push_back(action_b);
               //elog("action_b:${action_b}",("action_b",action_b));
            }
            if(pretty_input->act.account == N(cactus.token) && pretty_input->act.name == N(sell)){
               action_b.transcation_id = pretty_input->trx_id;
               pair_data_bean_sell sell = pretty_input->act.data_as<pair_data_bean_sell>();
               action_b.data = sell;
               result.data.actions.push_back(action_b);
               //elog("action_b:${action_b}",("action_b",action_b));
            }

         }
         if(actions.actions.size() == 0){
            result.code = 1;
            result.message = "do not get actions";             
         }

         return result;
      }

      asset read_only::exchange_state::convert_to_exchange( connector& c, asset in ) {

         real_type R(supply.get_amount());
         real_type C(c.balance.get_amount()+in.get_amount());
         real_type F(c.weight/1000.0);
         real_type T(in.get_amount());
         real_type ONE(1.0);

         real_type E = -R * (ONE - std::pow( ONE + T / C, F) );
         //print( "E: ", E, "\n");
         int64_t issued = int64_t(E);

         //supply.amount += issued;
         supply += asset(issued, supply.get_symbol());
         //c.balance.amount += in.get_amount();
         c.balance += in;
         return asset( issued, supply.get_symbol() );
      }

      asset read_only::exchange_state::convert_from_exchange( connector& c, asset in ) {
         //EOS_ASSERT( in.get_symbol()== supply.get_symbol(), "unexpected asset symbol input" ,("in.get_symbol",in.get_symbol())("supply.get_symbol",supply.get_symbol()));

         real_type R(supply.get_amount() - in.get_amount());
         real_type C(c.balance.get_amount());
         real_type F(1000.0/c.weight);
         real_type E(in.get_amount());
         real_type ONE(1.0);


        // potentially more accurate: 
        // The functions std::expm1 and std::log1p are useful for financial calculations, for example, 
        // when calculating small daily interest rates: (1+x)n
        // -1 can be expressed as std::expm1(n * std::log1p(x)). 
        // real_type T = C * std::expm1( F * std::log1p(E/R) );
         
         real_type T = C * (std::pow( ONE + E/R, F) - ONE);
         //print( "T: ", T, "\n");
         int64_t out = int64_t(T);

         //supply.amount -= in.amount;
         supply  -= in;
         c.balance -= asset(out, c.balance.get_symbol());

         return asset( out, c.balance.get_symbol() );
      }

      asset read_only::exchange_state::convert( asset from, symbol to ) {
         auto sell_symbol  = from.get_symbol();
         auto ex_symbol    = supply.get_symbol();
         auto base_symbol  = base.balance.get_symbol();
         auto quote_symbol = quote.balance.get_symbol();

         //print( "From: ", from, " TO ", asset( 0,to), "\n" );
         //print( "base: ", base_symbol, "\n" );
         //print( "quote: ", quote_symbol, "\n" );
         //print( "ex: ", supply.symbol, "\n" );

         if( sell_symbol != ex_symbol ) {
            if( sell_symbol == base_symbol ) {
               from = convert_to_exchange( base, from );
            } else if( sell_symbol == quote_symbol ) {
               from = convert_to_exchange( quote, from );
            } else { 
               //EOS_ASSERT( false, "invalid sell" ,("sell_symbol",sell_symbol)("ex_symbol",ex_symbol));
            }
         } else {
            if( to == base_symbol ) {
               from = convert_from_exchange( base, from ); 
            } else if( to == quote_symbol ) {
               from = convert_from_exchange( quote, from ); 
            } else {
               //EOS_ASSERT( false, "invalid conversion" ,("to",to)("base_symbol",base_symbol));
            }
         }

         if( to != from.get_symbol() )
            return convert( from, to );

         return from;
      }

      read_only::get_pairs_results read_only::get_pairs( const get_pairs_params& params )const {

         get_pairs_results result;
         result.code = 0;
         result.message = "ok"; 
         const auto& d = tokencoin->chain_plug->chain().db();
         //elog("pairs:${pairs}",("pairs",params.pairs));
         const auto code = N(cactus.token);
         const auto table = N(tokenmarket);
         for(const auto& pairString : params.pairs){
            //elog("pair:${pairString}",("pairString",pairString));
            uint64_t scope = ( eosio::chain::string_to_symbol( 0, pairString.c_str() ) >> 8 );
            const auto* t_id = d.find<chain::table_id_object, chain::by_code_scope_table>(boost::make_tuple( code, scope, table ));           
            if(t_id == nullptr) continue;
            const auto &idx = d.get_index<key_value_index, by_scope_primary>();           
            auto itr = idx.lower_bound(boost::make_tuple(t_id->id)); 
            if(itr == idx.end()) continue;

            exchange_state bal;
            fc::datastream<const char *> ds(itr->value.data(), itr->value.size());
            fc::raw::unpack(ds, bal);  

            pair p; 
            p.supply = bal.supply;
            p.base   = bal.base;  
            p.quote  = bal.quote;
            p.fee_amount       = bal.fee_amount; 
            //p.token_price_pair = 50;   cd 
            p.token_price_cny  = 50;    
            p.token_price_usd  = 10;  

            //for(int i = 0 ; i < )
            asset quan_out = asset(1000000000, bal.base.balance.get_symbol());
            //quan_out.amount = 1;
            //elog("quan_out:${quan_out} quote_symbol:${quote_symbol}",("quan_out",quan_out)("quote_symbol", bal.quote.balance.get_symbol()));
            asset base = bal.convert(quan_out, bal.quote.balance.get_symbol());
            //elog("base:${base}",("base",base));
            p.token_price_pair = double(base.get_amount()) / 1000000000.0;
            result.data.pairs.push_back(p);
         }

         if(result.data.pairs.size() == 0){
            result.code = 1;
            result.message = "do not get pairs";             
         }
         return result;
      }

      
      read_only::get_transactions_results read_only::get_transactions( const get_transactions_params& params )const {

         get_transactions_results result;
         result.code = 0;
         result.message = "ok"; 

         auto ro_api = app().find_plugin<history_plugin>()->get_read_only_api();
         auto actions = ro_api.get_actions({params.account_name, params.num_seq, params.skip_seq});
         //auto actions = ro_api.get_actions({params.account_name, -1, -20});
         result.data.last_irreversible_block = actions.last_irreversible_block;
         
         elog("actions:${actions}",("actions",actions));

         for(const auto& action: actions.actions ){
            action_bean action_b;
            action_b.block_num      = action.block_num;
            action_b.block_time     = action.block_time;
            action_b.seq_num        = action.account_action_seq;

            auto pretty_input = std::make_shared<action_trace>();
            auto resolver = make_resolver2(this,abi_serializer_max_time);
            try {
               abi_serializer::from_variant(action.action_trace, *pretty_input, resolver, abi_serializer_max_time);
            } EOS_RETHROW_EXCEPTIONS(chain::action_type_exception, "Invalid packed action_trace")

            //if(pretty_input->act.account == N(eosio.token) && pretty_input->act.name == N(transfer)){
            const char*  contract = params.contract.c_str();
            if(pretty_input->act.account == eosio::string_to_name(contract) && pretty_input->act.name == N(transfer)){
               action_b.transcation_id = pretty_input->trx_id;
               action_b.data = pretty_input->act.data_as<data_bean>();
               result.data.actions.push_back(action_b);
            }
         }
         if(actions.actions.size() == 0){
            result.code = 1;
            result.message = "do not get actions";             
         }

         return result;
      }

      read_only::get_sparklines_results read_only::get_sparklines( const get_sparklines_params& params )const {

         get_sparklines_results result;
         result.code = 0;
         result.message = "ok";

         if(params.token_symbol == "EOS"){       
             result.data.sparkline_token_png = "https://s2.coinmarketcap.com/generated/sparklines/web/7d/usd/1765.png";
         } else if (params.token_symbol == "BTC") {
             result.data.sparkline_token_png = "https://s2.coinmarketcap.com/generated/sparklines/web/7d/usd/1.png";
         } else if (params.token_symbol == "ETH") {
             result.data.sparkline_token_png = "https://s2.coinmarketcap.com/generated/sparklines/web/7d/usd/1027.png";
         } else if (params.token_symbol == "SYS") {
             result.data.sparkline_token_png = "https://s2.coinmarketcap.com/generated/sparklines/web/7d/usd/1765.png";
         } else {
             result.code = 1;
             result.message = "can't get sparklines";
         }

         return result;
      }

      read_only::get_account_asset_results read_only::get_account_asset( const get_account_asset_params& params )const {

         get_account_asset_results result;
         result.code = 0;
         result.message = "ok";
         result.data.account_name = params.account_name;
         result.data.account_icon = "do not have icon";
         const auto& acount = db.get_account(result.data.account_name);
         // if(a == nullptr){
         //     result.code = 1;
         //     result.message = "do NOT find account";
         //     return result;
         //  }

         const auto& d = tokencoin->chain_plug->chain().db();

         //get asset in eosio.token
         const auto token_code = N(eosio.token);
         const auto* t_id = d.find<chain::table_id_object, chain::by_code_scope_table>(boost::make_tuple( token_code, params.account_name, N(accounts) ));
         if( t_id == nullptr ){
            result.code = 1;
            result.message = "do NOT find account asset";
            return result;
         }

         const auto &idx = d.get_index<key_value_index, by_scope_primary>();

         decltype(t_id->id) next_tid(t_id->id._id + 1);
         auto lower = idx.lower_bound(boost::make_tuple(t_id->id));
         auto upper = idx.lower_bound(boost::make_tuple(next_tid));

         //vector<char> data;
         auto end = fc::time_point::now() + fc::microseconds(1000 * 10); /// 10ms max time
         unsigned int count = 0;
         auto itr = lower;
         for (itr = lower; itr != upper; ++itr) {

            if( itr->value.size() >= sizeof(asset) ){

               asset bal;
               fc::datastream<const char *> ds(itr->value.data(), itr->value.size());
               fc::raw::unpack(ds, bal);
               if( bal.get_symbol().valid()) {
                   asset_data ad;
                   ad.asset = bal;
                   ad.contract = "eosio.token";
                   result.data.account_assets.emplace_back(ad);
              }
            }

            if(fc::time_point::now() > end){
               break;
            }
         }


         //get asset in cactus.token
         const auto token= N(cactus.token);
         const auto* id = d.find<chain::table_id_object, chain::by_code_scope_table>(boost::make_tuple( token, params.account_name, N(accounts) ));
         if( id == nullptr ){
            result.code = 1;
            result.message = "do NOT find account asset";
            return result;
         }

         const auto &idx2 = d.get_index<key_value_index, by_scope_primary>();

         decltype(id->id) next_tid2(id->id._id + 1);
         lower = idx2.lower_bound(boost::make_tuple(id->id));
         upper = idx2.lower_bound(boost::make_tuple(next_tid2));

         //data;
         end = fc::time_point::now() + fc::microseconds(1000 * 10); /// 10ms max time
         count = 0;
         itr = lower;
         for (itr = lower; itr != upper; ++itr) {

            if( itr->value.size() >= sizeof(asset) ){

               asset b;
               fc::datastream<const char *> ds(itr->value.data(), itr->value.size());
               fc::raw::unpack(ds, b);
               if( b.get_symbol().valid()) {
                   asset_data a;
                   a.asset = b;
                   a.contract = "cactus.token";
                   result.data.account_assets.emplace_back(a);
              }
            }

            if(fc::time_point::now() > end){
               break;
            }
         }
         return result;

      }

      read_only::get_account_app_results read_only::get_account_app( const get_account_app_params& params )const {

         get_account_app_results result;
         result.code = 0;
         result.message = "ok";
         result.data.account_name = params.account_name;
         const auto& db = tokencoin->chain_plug->chain();
         const auto& a = db.get_account(result.data.account_name);
         // if(a == NULL){
         //    result.code = 1;
         //    result.message = "do NOT find account";
         //    return result;
         // }

         result.data.last_unstaking_time          = a.creation_date;

         const auto& d = db.db();
         const auto& permissions = d.get_index<permission_index,by_owner>();
         auto perm = permissions.lower_bound( boost::make_tuple( params.account_name ) );
         while( perm != permissions.end() && perm->owner == params.account_name ) {
            /// TODO: lookup perm->parent name
            name parent;

            // Don't lookup parent if null
            if( perm->parent._id ) {
               const auto* p = d.find<permission_object,by_id>( perm->parent );
               if( p ) {
                  FC_ASSERT(perm->owner == p->owner, "Invalid parent");
                  parent = p->name;
               }
            }

            result.data.permissions.push_back( permission{ perm->name, parent, perm->auth.to_authority() } );
            ++perm;
         }

         const auto& code_account = d.get<account_object,by_name>( N(eosio) );
         //const abi_def abi = get_abi( db, N(eosio) );
         abi_def abi;
         if( abi_serializer::to_abi(code_account.abi, abi) ) {
            abi_serializer abis( abi , fc::microseconds(config::default_abi_serializer_max_time_ms));
            //get_table_rows_ex<key_value_index, by_scope_primary>(p,abi);

            const auto token_code = N(eosio.token);

            const auto* t_id = d.find<chain::table_id_object, chain::by_code_scope_table>(boost::make_tuple( token_code, params.account_name, N(accounts) ));
            if( t_id != nullptr ) {
               const auto &idx = d.get_index<key_value_index, by_scope_primary>();
               auto it = idx.find(boost::make_tuple( t_id->id, symbol().to_symbol_code() ));
               if( it != idx.end() && it->value.size() >= sizeof(asset) ) {
                  asset bal;
                  fc::datastream<const char *> ds(it->value.data(), it->value.size());
                  fc::raw::unpack(ds, bal);

                  if( bal.get_symbol().valid() && bal.get_symbol() == symbol() ) {
                     result.data.eos_balance       = bal;
                     result.data.staked_balance    = bal;
                     result.data.unstaking_balance = bal;
                  }
               }
            }


         }

         return result;
      }

   } /// tokencoin_apis



} /// namespace eosio