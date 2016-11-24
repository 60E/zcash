// Copyright (c) 2015 G. Andrew Stone
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chain.h"
#include "clientversion.h"
#include "chainparams.h"
#include "miner.h"
#include "consensus/consensus.h"
#include "consensus/params.h"
#include "consensus/validation.h"
#include "main.h"
#include "net.h"
#include "primitives/block.h"
#include "rpcserver.h"
#include "timedata.h"
#include "tinyformat.h"
#include "txmempool.h"
#include "utilstrencodings.h"
#include "ui_interface.h"
#include "util.h"
#include "validationinterface.h"
#include "version.h"

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <iomanip>
#include <boost/thread.hpp>
#include <inttypes.h>
#include <queue>
#include "unlimited.h"

using namespace json_spirit;
using namespace std;

const uint32_t BIP_009_MASK = 0x20000000;
const uint32_t BASE_VERSION = 0x20000000;
const uint32_t FORK_BIT_2MB = 0x10000000;  // Vote for 2MB fork

uint64_t maxGeneratedBlock = DEFAULT_MAX_GENERATED_BLOCK_SIZE;
unsigned int excessiveBlockSize = DEFAULT_EXCESSIVE_BLOCK_SIZE;
unsigned int excessiveAcceptDepth = DEFAULT_EXCESSIVE_ACCEPT_DEPTH;
uint32_t  blockVersion = 0;

void UnlimitedSetup(void)
{
    maxGeneratedBlock = GetArg("-blockmaxsize", maxGeneratedBlock);
    blockVersion = GetArg("-blockversion", blockVersion);
    excessiveBlockSize = GetArg("-excessiveblocksize", excessiveBlockSize);
    excessiveAcceptDepth = GetArg("-excessiveacceptdepth", excessiveAcceptDepth);

    //settingsToUserAgentString();
}

int chainContainsExcessive(const CBlockIndex* blk, unsigned int goBack)
{
    if (goBack == 0)
        goBack = excessiveAcceptDepth+EXCESSIVE_BLOCK_CHAIN_RESET;

    for (unsigned int i = 0; i < goBack; i++, blk = blk->pprev)
    {
        if (!blk)
            break; // we hit the beginning
        if (blk->nStatus & BLOCK_EXCESSIVE)
            return true;
    }
    return false;
}

bool CheckExcessive(const CBlock& block, uint64_t blockSize, uint64_t nSigOps, uint64_t nTx)
{
    if (blockSize > excessiveBlockSize) {
        LogPrintf("Excessive block: ver:%x time:%d size: %" PRIu64 " Tx:%" PRIu64 " Sig:%d  :too many bytes\n", block.nVersion, block.nTime, blockSize, nTx, nSigOps);
        return true;
    }

    LogPrintf("Acceptable block: ver:%x time:%d size: %" PRIu64 " Tx:%" PRIu64 " Sig:%d\n", block.nVersion, block.nTime, blockSize, nTx, nSigOps);
    return false;
}

int32_t UnlimitedComputeBlockVersion(const CBlockIndex* pindexPrev, const Consensus::Params& params,uint32_t nTime)
{
    if (blockVersion != 0)  // BU: allow override of block version
    {
        return blockVersion;
    }

    int32_t nVersion = CBlockHeader::CURRENT_VERSION;

    // turn BIP109 off by default by commenting this out:
    // if (nTime <= params.SizeForkExpiration())
    //	  nVersion |= FORK_BIT_2MB;

    return nVersion;
}

Value getexcessiveblock(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
                "getexcessiveblock\n"
                        "\nReturn the excessive block size and accept depth."
                        "\nResult\n"
                        "  excessiveBlockSize (integer) block size in bytes\n"
                        "  excessiveAcceptDepth (integer) if the chain gets this much deeper than the excessive block, then accept the chain as active (if it has the most work)\n"
                        "\nExamples:\n" +
                HelpExampleCli("getexcessiveblock", "") + HelpExampleRpc("getexcessiveblock", ""));

    Object ret;
    ret.push_back(Pair("excessiveBlockSize", (uint64_t)excessiveBlockSize));
    ret.push_back(Pair("excessiveAcceptDepth", (uint64_t)excessiveAcceptDepth));
    return ret;
}

Value setexcessiveblock(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 2 || params.size() >= 3)
        throw runtime_error(
                "setexcessiveblock blockSize acceptDepth\n"
                        "\nSet the excessive block size and accept depth.  Excessive blocks will not be used in the active chain or relayed until they are several blocks deep in the blockchain.  This discourages the propagation of blocks that you consider excessively large.  However, if the mining majority of the network builds upon the block then you will eventually accept it, maintaining consensus."
                        "\nResult\n"
                        "  blockSize (integer) excessive block size in bytes\n"
                        "  acceptDepth (integer) if the chain gets this much deeper than the excessive block, then accept the chain as active (if it has the most work)\n"
                        "\nExamples:\n" +
                HelpExampleCli("getexcessiveblock", "") + HelpExampleRpc("getexcessiveblock", ""));

    unsigned int ebs=0;
    if (params[0].is_uint64())
        ebs = params[0].get_int64();
    else {
        string temp = params[0].get_str();
        if (temp[0] == '-') boost::throw_exception( boost::bad_lexical_cast() );
        ebs = boost::lexical_cast<unsigned int>(temp);
    }

    if (ebs < maxGeneratedBlock)
    {
        std::ostringstream ret;
        ret << "Sorry, your maximum mined block (" << maxGeneratedBlock << ") is larger than your proposed excessive size (" << ebs << ").  This would cause you to orphan your own blocks.";
        throw runtime_error(ret.str());
    }
    excessiveBlockSize = ebs;

    if (params[1].is_uint64())
        excessiveAcceptDepth = params[1].get_int64();
    else {
        string temp = params[1].get_str();
        if (temp[0] == '-') boost::throw_exception( boost::bad_lexical_cast() );
        excessiveAcceptDepth = boost::lexical_cast<unsigned int>(temp);
    }

    //settingsToUserAgentString();
    std::ostringstream ret;
    ret << "Excessive Block set to " << excessiveBlockSize << " bytes.  Accept Depth set to " << excessiveAcceptDepth << " blocks.";
    return ret.str();
}

Value getminingmaxblock(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
                "getminingmaxblock\n"
                        "\nReturn the max generated (mined) block size"
                        "\nResult\n"
                        "      (integer) maximum generated block size in bytes\n"
                        "\nExamples:\n" +
                HelpExampleCli("getminingmaxblock", "") + HelpExampleRpc("getminingmaxblock", ""));

    return maxGeneratedBlock;
}


Value setminingmaxblock(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
                "setminingmaxblock blocksize\n"
                        "\nSet the maximum number of bytes to include in a generated (mined) block.  This command does not turn generation on/off.\n"
                        "\nArguments:\n"
                        "1. blocksize         (integer, required) the maximum number of bytes to include in a block.\n"
                        "\nExamples:\n"
                        "\nSet the generated block size limit to 8 MB\n" +
                HelpExampleCli("setminingmaxblock", "8000000") +
                "\nCheck the setting\n" + HelpExampleCli("getminingmaxblock", ""));

    uint64_t arg = 0;
    if (params[0].is_uint64())
        arg = params[0].get_int64();
    else {
        string temp = params[0].get_str();
        if (temp[0] == '-') boost::throw_exception( boost::bad_lexical_cast() );
        arg = boost::lexical_cast<uint64_t>(temp);
    }

    // I don't want to waste time testing edge conditions where no txns can fit in a block, so limit the minimum block size
    if (arg < 1000)
        throw runtime_error("max generated block size must be greater than 1KB");

    maxGeneratedBlock = arg;

    Object ret;
    ret.push_back(Pair("maxGeneratedBlock", (uint64_t)maxGeneratedBlock));
    return ret;
}

Value getblockversion(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
                "getblockversion\n"
                        "\nReturn the block version used when mining."
                        "\nResult\n"
                        "      (integer) block version number\n"
                        "\nExamples:\n" +
                HelpExampleCli("getblockversion", "") + HelpExampleRpc("getblockversion", ""));
    const CBlockIndex* pindex = chainActive.Tip();
    return UnlimitedComputeBlockVersion(pindex, Params().GetConsensus(),pindex->nTime);
}

Value setblockversion(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
                "setblockversion blockVersionNumber\n"
                        "\nSet the block version number.\n"
                        "\nArguments:\n"
                        "1. blockVersionNumber         (integer, hex integer, 'BIP109', 'BASE' or 'default'.  Required) The block version number.\n"
                        "\nExamples:\n"
                        "\nVote for 2MB blocks\n" +
                HelpExampleCli("setblockversion", "BIP109") +
                "\nCheck the setting\n" + HelpExampleCli("getblockversion", ""));

    uint32_t arg = 0;

    string temp = params[0].get_str();
    if (temp == "default")
    {
        arg = 0;
    }
    else if (temp == "BIP109")
    {
        arg = BASE_VERSION | FORK_BIT_2MB;
    }
    else if (temp == "BASE")
    {
        arg = BASE_VERSION;
    }
    else if ((temp[0] == '0') && (temp[1] == 'x'))
    {
        std::stringstream ss;
        ss << std::hex << (temp.c_str()+2);
        ss >> arg;
    }
    else
    {
        arg = boost::lexical_cast<unsigned int>(temp);
    }

    blockVersion = arg;

    std::ostringstream ret;
    ret << "blockVersion set to " << blockVersion;
    return ret.str();
}

extern Value getminercomment(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
                "getminercomment\n"
                        "\nReturn the comment that will be put into each mined block's coinbase\n transaction after the hcash parameters."
                        "\nResult\n"
                        "  minerComment (string) miner comment\n"
                        "\nExamples:\n" +
                HelpExampleCli("getminercomment", "") + HelpExampleRpc("getminercomment", ""));

    std::ostringstream ret;
    ret << "Method disabled";
    return ret.str();
}

extern Value setminercomment(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
                "setminercomment\n"
                        "\nSet the comment that will be put into each mined block's coinbase\n transaction after the hcash parameters.\n Comments that are too long will be truncated."
                        "\nExamples:\n" +
                HelpExampleCli("setminercomment", "\"hcash is fundamentally emergent consensus\"") + HelpExampleRpc("setminercomment", "\"hcash is fundamentally emergent consensus\""));

    //minerComment = params[0].getValStr();
    std::ostringstream ret;
    ret << "Method disabled";
    return ret.str();
}
