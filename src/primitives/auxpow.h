// Copyright (c) 2009-2010 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_AUXPOW_H
#define BITCOIN_AUXPOW_H

//#include <wallet/wallet.h>
//#include "main.h"
#include <utilstrencodings.h>
#include "consensus/params.h"
#include "primitives/transaction.h"
#include "serialize.h"
#include "uint256.h"
#include "hash.h"
#include "util.h"

class CBlock;
class CBlockHeader;
class CBlockIndex;
/** A transaction with a merkle branch linking it to the block chain. */
class CMerkleTx : public CTransaction
{
private:
    int GetDepthInMainChainINTERNAL(const CBlockIndex* &pindexRet) const;

public:
    uint256 hashBlock;
    std::vector<uint256> vMerkleBranch;
    int nIndex;

    // memory only
    mutable bool fMerkleVerified;


    CMerkleTx()
    {
        Init();
    }

    CMerkleTx(const CTransaction& txIn) : CTransaction(txIn)
    {
        Init();
    }

    void Init()
    {
        hashBlock = uint256();
        nIndex = -1;
        fMerkleVerified = false;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(*(CTransaction*)this);
        nVersion = this->nVersion;
        READWRITE(hashBlock);
        READWRITE(vMerkleBranch);
        READWRITE(nIndex);
    }

    int SetMerkleBranch(const CBlock& block);


    /**
     * Return depth of transaction in blockchain:
     * -1  : not in blockchain, and not in memory pool (conflicted transaction)
     *  0  : in memory pool, waiting to be included in a block
     * >=1 : this many blocks deep in the main chain
     */
    int GetDepthInMainChain(const CBlockIndex* &pindexRet) const;
    int GetDepthInMainChain() const { const CBlockIndex *pindexRet; return GetDepthInMainChain(pindexRet); }
    bool IsInMainChain() const { const CBlockIndex *pindexRet; return GetDepthInMainChainINTERNAL(pindexRet) > 0; }
    int GetBlocksToMaturity() const;
    bool AcceptToMemoryPool(bool fLimitFree=true, bool fRejectAbsurdFee=true);
};

class CParentBlockHeader
{
public:
    // header
    static const int CURRENT_VERSION=2;
    int nVersion;
    uint256 hashPrevBlock;
    uint256 hashMerkleRoot;
    unsigned int nTime;
    unsigned int nBits;
    unsigned int nNonce;

    CParentBlockHeader()
    {
        SetNull();
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(this->nVersion);
        nVersion = this->nVersion;
        READWRITE(hashPrevBlock);
        READWRITE(hashMerkleRoot);
        READWRITE(nTime);
        READWRITE(nBits);
        READWRITE(nNonce);
    }

    void SetNull()
    {
        nVersion = CParentBlockHeader::CURRENT_VERSION;
        hashPrevBlock.SetNull();
        hashMerkleRoot.SetNull();
        nTime = 0;
        nBits = 0;
        nNonce = 0;
    }

    bool IsNull() const
    {
        return (nTime == 0);
    }

    uint256 GetHash() const
    {
        return Hash(BEGIN(nVersion), END(nNonce));
    }
};


class CAuxPow
{
public:
    CMerkleTx mMerkleTx;
    // Merkle branch with root vchAux
    // root must be present inside the coinbase
    std::vector<uint256> vChainMerkleBranch;
    // Index of chain in chains merkle tree
    int nChainIndex;
    CParentBlockHeader vParentBlockHeader;

    CAuxPow();
    CAuxPow(const CTransaction& txIn);
    
    bool Check(uint256 hashAuxBlock, int nChainID) const;
    
    bool IsNull() const
    {
        return vParentBlockHeader.IsNull();
    }

    uint256 GetParentBlockHash()
    {
        return vParentBlockHeader.GetHash();
    }

    uint256 GetPoWHash() const;
    
    void print() const
    {
        printf("CAuxPow(version = %d, hash=%s, hashPrevBlock=%s, hashMerkleRoot=%s, nTime=%u, nBits=%08x, nNonce=%u)\n", 
            vParentBlockHeader.nVersion, vParentBlockHeader.GetHash().ToString().c_str(),
            vParentBlockHeader.hashPrevBlock.ToString().c_str(),
            vParentBlockHeader.hashMerkleRoot.ToString().c_str(),
            vParentBlockHeader.nTime, vParentBlockHeader.nBits, vParentBlockHeader.nNonce
            );
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(mMerkleTx);
        READWRITE(vChainMerkleBranch);
        READWRITE(nChainIndex);
        READWRITE(vParentBlockHeader);
    }

};
//
//template <typename Stream>
//int ReadWriteAuxPow(Stream& s, const boost::shared_ptr<CAuxPow>& auxpow, int nType, int nVersion, CSerActionGetSerializeSize ser_action)
//{
//    if (nVersion & (1 << 8))
//    {
//        return ::GetSerializeSize(*auxpow, nType, nVersion);
//    }
//    return 0;
//}
//
//template <typename Stream>
//int ReadWriteAuxPow(Stream& s, const boost::shared_ptr<CAuxPow>& auxpow, int nType, int nVersion, CSerActionSerialize ser_action)
//{
//    if (nVersion & (1 << 8))
//    {
//        return SerReadWrite(s, *auxpow, nType, nVersion, ser_action);
//    }
//    return 0;
//}
//
//template <typename Stream>
//int ReadWriteAuxPow(Stream& s, boost::shared_ptr<CAuxPow>& auxpow, int nType, int nVersion, CSerActionUnserialize ser_action)
//{
//    if (nVersion & (1 << 8))
//    {
//        auxpow.reset(new CAuxPow());
//        return SerReadWrite(s, *auxpow, nType, nVersion, ser_action);
//    }
//    else
//    {
//        auxpow.reset();
//        return 0;
//    }
//}

//extern void RemoveMergedMiningHeader(std::vector<unsigned char>& vchAux);
//extern CScript MakeCoinbaseWithAux(unsigned int nBits, unsigned int nExtraNonce, std::vector<unsigned char>& vchAux);
extern void IncrementExtraNonceWithAux(CBlock* pblock, CBlockIndex* pindexPrev, unsigned int& nExtraNonce, uint64_t& nPrevTime, std::vector<unsigned char>& vchAux, int64_t time);
#endif


