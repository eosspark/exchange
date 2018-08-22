#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <exchange_state.hpp>
#include <string>


namespace pegasus {
   using std::string;
   using eosio::asset;
   using eosio::symbol_name;

   //static constexpr uint64_t     system_token_symbol = CORE_SYMBOL;

   class token : public eosio::contract {

       public:
         token( account_name s );
         ~token();

       public:
        void create( account_name issuer, 
                     asset maximum_supply, 
                     asset exchange_base, 
                     uint64_t fee_amount, 
                     uint64_t open_transfer );
        void transfer( account_name from,
                       account_name to,
                       asset        quantity,
                       string       memo );
        void buy( account_name from, 
                  asset base_quant, 
                  asset asset_symbol, 
                  account_name feeto );
        void sell( account_name from, 
                   asset asset_quant, 
                   asset base_symbol, 
                   account_name feeto );
        inline asset get_supply( symbol_name sym )const;         
        inline asset get_balance( account_name owner, symbol_name sym )const;

       private:
         struct account {
            asset    balance;
            uint64_t primary_key()const { return balance.symbol.name(); }
            EOSLIB_SERIALIZE( account, (balance) )
         };

         struct currency_stats {
            asset          supply;
            asset          max_supply;
            account_name   issuer;
            uint64_t       open_transfer;
            uint64_t primary_key()const { return supply.symbol.name(); }           
            EOSLIB_SERIALIZE( currency_stats, (supply)(max_supply)(issuer)(open_transfer) )
         };

         typedef eosio::multi_index<N(accounts), account> accounts;
         typedef eosio::multi_index<N(stat), currency_stats> stats;

         void sub_balance( account_name owner, asset value );
         void add_balance( account_name owner, asset value, account_name ram_payer );

         //tokenmarket _tokenmarket;
         //stats _statstable;
   };  

   asset token::get_supply( symbol_name sym )const
   {
      stats statstable( _self, sym );
      const auto& st = statstable.get( sym );
      return st.supply;          
   } 

   asset token::get_balance( account_name owner, symbol_name sym )const
   {
      accounts accountstable( _self, owner );
      const auto& ac = accountstable.get( sym );
      return ac.balance;
   }

} /// namespace pegasus
