// Copyright (c) 2015 G. Andrew Stone
// Copyright (c) 2016 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once
#ifndef BITCOIN_UNLIMITED_H
#define BITCOIN_UNLIMITED_H

#include "net.h"
#include "consensus/validation.h"
#include "consensus/params.h"
#include <vector>
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"

enum {
    DEFAULT_MAX_GENERATED_BLOCK_SIZE = 2000000,
    DEFAULT_EXCESSIVE_ACCEPT_DEPTH = 4,
    DEFAULT_EXCESSIVE_BLOCK_SIZE = 16000000,
    DEFAULT_MAX_MESSAGE_SIZE_MULTIPLIER = 10,
    MAX_COINBASE_SCRIPTSIG_SIZE = 100,
    EXCESSIVE_BLOCK_CHAIN_RESET = 6*24,  // After 1 day of non-excessive blocks, reset the checker
};

extern uint64_t maxGeneratedBlock;
extern unsigned int excessiveBlockSize;
extern unsigned int excessiveAcceptDepth;
extern uint32_t  blockVersion;

extern void UnlimitedSetup(void);

extern json_spirit::Value getexcessiveblock(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value setexcessiveblock(const json_spirit::Array& params, bool fHelp);

extern json_spirit::Value getminingmaxblock(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value setminingmaxblock(const json_spirit::Array& params, bool fHelp);

// Get and set the custom string that miners can place into the coinbase transaction
extern json_spirit::Value getminercomment(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value setminercomment(const json_spirit::Array& params, bool fHelp);

// Get and set the generated (mined) block version.  USE CAREFULLY!
extern json_spirit::Value getblockversion(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value setblockversion(const json_spirit::Array& params, bool fHelp);

extern int chainContainsExcessive(const CBlockIndex* blk, unsigned int goBack = 0);
extern bool CheckExcessive(const CBlock& block, uint64_t blockSize, uint64_t nSigOps, uint64_t nTx);

extern int32_t UnlimitedComputeBlockVersion(const CBlockIndex* pindexPrev, const Consensus::Params& params,uint32_t nTime);

#endif

