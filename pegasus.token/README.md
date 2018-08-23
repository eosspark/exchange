# pegasus.token Decentralization Exchange

1.set contract@eosio.code to user@active and contract@active

2.cleos set contract cactus.token  pegasus.token -p cactus.token 

3.cleos push action cactus.token create '["cactus","100000000000.0000 LSK","100000000000.0000 EOS",200,0]' -p cactus 

4.cleos push action cactus.token buy '["xiaoyu","100000.0000 EOS","0.0000 LSK","cactus"]' -p xiaoyu 

5.cleos push action cactus.token sell '["xiaoyu","9000000.0000 LSK","0.0000 EOS","cactus"]' -p xiaoyu

6.cleos get table cactus.token LSKEOS tokenmarket

{
  "rows": [{
      "supply": "100000000000000 LSKEOS",
      "base": {
        "balance": "99999701503.9228 LSK",
        "weight": "0.50000000000000000"
      },
      "quote": {
        "balance": "100000298500.0000 EOS",
        "weight": "0.50000000000000000"
      },
      "fee_amount": 200
    }
  ],
  "more": false
}

7.cleos get table cactus.token xiaoyu accounts

{
  "rows": [{
      "balance": "398001.6387 PEG"
    },{
      "balance": "99498.0593 LSK"
    },{
      "balance": "90497069.3080 BTS"
    },{
      "balance": "4800.6043 CTS"
    }
  ],
  "more": false
}