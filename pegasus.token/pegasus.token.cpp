#pragma once

#include <eosiolib/eosio.hpp>
#include <pegasus.token.hpp>
#include <exchange_state.cpp>
#include <eosio.token/eosio.token.hpp>

namespace pegasus {

symbol_type symbol2pair(symbol_type symbol, symbol_type symbol_base)
{
  //uint8_t precision = (uint8_t)symbol.precision();
  uint8_t precision = 0;
  uint64_t name = symbol.name();
  uint64_t name_base = symbol_base.name(); 
  uint32_t name_total = symbol.name_length() + symbol_base.name_length();

  char name_new[name_total];
  auto sym = symbol.value;
  sym >>= 8;
  int k = 0;
  for( int i = 0; i < 7; ++i ) {
    char c1 = (char)(sym & 0xff);
    if( !c1 ) break;
    name_new[k++] = c1;
    sym >>= 8;
  }

  sym = symbol_base.value;
  sym >>= 8;
  for ( int j = 0; j < 7; ++j){
    char c2 = (char)(sym & 0xff);
    if( !c2 ) break;
    name_new[j + k] = c2;
    sym >>= 8;
  }

  return ::eosio::string_to_symbol(precision,name_new);
}


token::token( account_name self )
:contract(self)
{
   eosio::print( "construct system\n" );
}

token::~token() 
{
   //eosio::print( "destruct system\n" );
   //eosio_exit(0);
} 

void token::create( account_name issuer, 
                    asset maximum_supply, 
                    asset exchange_base, 
                    uint64_t fee_amount, 
                    uint64_t open_transfer )
{
   require_auth( issuer );

   //uint64_t fee_amount = 200;
   //auto core = symbol2core(maximum_supply.symbol, "CORE");
   auto pair = symbol2pair(maximum_supply.symbol, exchange_base.symbol);

   pair.print();
   auto sym = maximum_supply.symbol;
   eosio_assert( sym.is_valid(), "invalid symbol name" );
   eosio_assert( maximum_supply.is_valid(), "invalid supply");
   eosio_assert( maximum_supply.amount > 0, "max-supply must be positive");
   eosio_assert( fee_amount > 0, "fee amount must bigger than 0");

   stats statstable( _self, sym.name() );
   auto existing = statstable.find( sym.name());
   eosio_assert( existing == statstable.end(), "token with symbol already exists" );

   statstable.emplace(_self, [&]( auto& s ) {
      s.supply.symbol = maximum_supply.symbol;
      s.max_supply    = maximum_supply;
      s.issuer        = issuer;
      s.open_transfer  = open_transfer;
   });

   tokenmarket market( _self, pair.name() );
   auto itr = market.find(pair.name());
   eosio_assert( itr == market.end(), "pair already exists" );

   auto exchange_base_supply   = eosio::token(N(eosio.token)).get_supply(eosio::symbol_type(exchange_base.symbol).name()).amount;
   eosio_assert( exchange_base_supply > 0, "exchange_base_supply must bigger than 0 " );
   eosio_assert( exchange_base_supply > exchange_base.amount, "exchange_base_supply must bigger than exchange_base amount" );

   itr = market.emplace( _self, [&]( auto& m ) {
      m.supply.amount = 100000000000000ll;
      m.supply.symbol = pair;
      m.base.balance.amount = maximum_supply.amount;
      m.base.balance.symbol = maximum_supply.symbol;
      m.quote.balance.amount = exchange_base.amount;
      m.quote.balance.symbol = exchange_base.symbol;
      m.fee_amount           = fee_amount;
   });
}

void token::transfer( account_name from,
                      account_name to,
                      asset        quantity,
                      string       memo )
{
   eosio_assert( from != to, "cannot transfer to self" );
   require_auth( from );
   eosio_assert( is_account( to ), "to account does not exist");
   auto sym = quantity.symbol;
   stats statstable( _self, sym.name() );
   const auto& st = statstable.get( sym.name() );

   require_recipient( from );
   require_recipient( to );

   eosio_assert( quantity.is_valid(), "invalid quantity" );
   eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
   eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
   eosio_assert( st.open_transfer > 0, "this asset can't transfer" );  
   eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

   sub_balance( from, quantity );
   add_balance( to, quantity, from );
}

void token::sub_balance( account_name owner, asset value ) 
{
   accounts from_acnts( _self, owner );

   const auto& from = from_acnts.get( value.symbol.name(), "no balance object found" );
   eosio_assert( from.balance.amount >= value.amount, "overdrawn balance" );

   if( from.balance.amount == value.amount ) {
      from_acnts.erase( from );
   } else {
      from_acnts.modify( from, owner, [&]( auto& a ) {
          a.balance -= value;
      });
   }
}

void token::add_balance( account_name owner, asset value, account_name payer )
{
   accounts to_acnts( _self, owner );
   auto to = to_acnts.find( value.symbol.name() );
   if( to == to_acnts.end() ) {
      to_acnts.emplace( payer, [&]( auto& a ){
        a.balance = value;
      });
   } else {
      to_acnts.modify( to, 0, [&]( auto& a ) {
        a.balance += value;
      });
   }
}

/**
*  This action will buy an exact amount of ram and bill the payer the current market price.
*/
// void token::buytokens( account_name payer, asset quant) {
//   auto core = symbol2core(quant.symbol, "CORE");

//   tokenmarket market( _self, core.name() );
//   auto itr = market.find(core);
//   auto tmp = *itr;
//   eosio_assert( itr == market.end(), "token market doesn't exists" );

//   auto sysout = tmp.convert( quant, CORE_SYMBOL );
//   buy( payer, sysout, quant);
// }

void token::buy( account_name from, asset quant, asset symbol, account_name feeto) 
{
   //eosio::print( "token::buy line 130\n" );
   require_auth( from );
   eosio_assert( quant.amount > 0, "must purchase a positive amount" );
   eosio_assert( symbol.amount == 0, "symbol amount must 0" );

   //assettoken transfer
   asset    quan_out;
   //uint64_t fee_amount;
   auto fee             = quant;
   auto quant_after_fee = quant;

   auto pair = symbol2pair(symbol.symbol, quant.symbol);
   tokenmarket market( _self, pair.name() );
   auto itr = market.find( pair.name() );
   eosio_assert( itr != market.end(), "pair does not exist" );
   market.modify( itr, 0, [&]( auto& es ) {
       fee.amount = ( fee.amount + es.fee_amount - 1 ) / es.fee_amount; /// .5% fee (round up)
       quant_after_fee.amount -= fee.amount;
       quan_out.amount = es.convert( quant_after_fee, es.base.balance.symbol ).amount;
       quan_out.symbol = es.base.balance.symbol;
   });

   INLINE_ACTION_SENDER(eosio::token, transfer)( N(eosio.token), {from,N(active)},
      { from, _self, quant_after_fee, std::string("buy token") } );
   //eosio::print( "token::buy line 146\n" );
   if( fee.amount > 0 ) {
      INLINE_ACTION_SENDER(eosio::token, transfer)( N(eosio.token), {from,N(active)},
                                                    { from, feeto, fee, std::string("token fee") } );
   }

   add_balance( from, quan_out, _self);

   eosio_assert( quan_out.amount > 0, "must reserve a positive amount" );

   auto sym = quan_out.symbol;
   stats statstable( _self, sym.name());
   auto st = statstable.find( sym.name() );
   eosio_assert( st != statstable.end(), "token with symbol doesn't exists" );
   statstable.modify( st, 0, [&]( auto& s ) {
       s.supply += quan_out;
   });

   require_recipient( from );

}

void token::sell( account_name from, asset quant, asset symbol, account_name feeto )
{
   require_auth( from );
   eosio_assert( symbol.amount == 0, "symbol amount must 0" );
   eosio_assert( quant.is_valid(), "invalid quantity" );
   eosio_assert( quant.amount > 0, "must transfer positive quantity" );

   //assettoken transfer 
   asset    tokens_out;
   uint64_t fee_amount;
   auto pair = symbol2pair(quant.symbol, symbol.symbol);
   tokenmarket market( _self, pair.name() );
   auto itr = market.find(pair.name());
   eosio_assert( itr != market.end(), "pair does not exist" );
   market.modify( itr, 0, [&]( auto& es ) {
        tokens_out = es.convert( quant, symbol.symbol);
        fee_amount = es.fee_amount;
   });

   sub_balance( from, quant );

   //basetoken eosio.token transfer
   eosio_assert( tokens_out.amount > 1, "token amount received from selling pair is too low" );
   auto fee    = ( tokens_out.amount + fee_amount - 1 ) / fee_amount; /// .5% fee (round up) 
   tokens_out.amount -= fee;
   INLINE_ACTION_SENDER(eosio::token, transfer)( N(eosio.token), {_self,N(active)},
                                                    { _self, from, asset(tokens_out.amount, symbol.symbol), std::string("sell token") } );
   if( fee > 0 ) {
      INLINE_ACTION_SENDER(eosio::token, transfer)( N(eosio.token), {from,N(active)},
            { from, feeto, asset(fee, symbol.symbol ), std::string("sell fee") } );
   }

   auto sym = quant.symbol;
   stats statstable( _self, sym.name() );
   auto st = statstable.find( sym.name() );
   eosio_assert( st != statstable.end(), "token with symbol doesn't exists" );
   eosio_assert( quant.symbol == st->supply.symbol, "symbol precision mismatch" );
   statstable.modify( st, 0, [&]( auto& s ) {
       s.supply -= quant;
   });

   require_recipient( from );

}


}/// namespace pegasus

//EOSIO_ABI( pegasus::token, (create)(transfer)(buytokens)(buy)(sell) )
EOSIO_ABI( pegasus::token, (create)(transfer)(buy)(sell) )
