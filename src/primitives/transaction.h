// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PRIMITIVES_TRANSACTION_H
#define BITCOIN_PRIMITIVES_TRANSACTION_H

#include "amount.h"
#include "script/script.h"
#include "serialize.h"
#include "uint256.h"
#include "pubkey.h"

#include <secp256k1_rangeproof.h>

static const int SERIALIZE_TRANSACTION_NO_WITNESS = 0x40000000;

static const int WITNESS_SCALE_FACTOR = 4;

static const uint8_t PARTICL_BLOCK_VERSION = 0xA0;
static const uint8_t PARTICL_TXN_VERSION = 0xA0;
static const uint8_t MAX_PARTICL_TXN_VERSION = 0xBF;


enum OutputTypes
{
    OUTPUT_NULL             = 0, // marker for CCoinsView
    OUTPUT_STANDARD         = 1,
    OUTPUT_CT               = 2,
    OUTPUT_RINGCT           = 3,
    OUTPUT_DATA             = 4,
};

enum TransactionTypes
{
    TXN_STANDARD            = 0,
    TXN_COINBASE            = 1,
    TXN_COINSTAKE           = 2,
};

enum DataOutputTypes
{
    DO_NULL                 = 0, // reserved
    DO_NARR_PLAIN           = 1,
    DO_NARR_CRYPT           = 2,
    DO_STEALTH              = 3,
    DO_STEALTH_PREFIX       = 4,
    DO_VOTE                 = 5,
    DO_FEE                  = 6,
};

inline bool IsParticlTxVersion(int nVersion)
{
    return (nVersion & 0xFF) >= PARTICL_TXN_VERSION;
}

/** An outpoint - a combination of a transaction hash and an index n into its vout */
class COutPoint
{
public:
    uint256 hash;
    uint32_t n;

    COutPoint() { SetNull(); }
    COutPoint(uint256 hashIn, uint32_t nIn) { hash = hashIn; n = nIn; }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(hash);
        READWRITE(n);
    }

    void SetNull() { hash.SetNull(); n = (uint32_t) -1; }
    bool IsNull() const { return (hash.IsNull() && n == (uint32_t) -1); }

    friend bool operator<(const COutPoint& a, const COutPoint& b)
    {
        int cmp = a.hash.Compare(b.hash);
        return cmp < 0 || (cmp == 0 && a.n < b.n);
    }

    friend bool operator==(const COutPoint& a, const COutPoint& b)
    {
        return (a.hash == b.hash && a.n == b.n);
    }

    friend bool operator!=(const COutPoint& a, const COutPoint& b)
    {
        return !(a == b);
    }

    std::string ToString() const;
};

/** An input of a transaction.  It contains the location of the previous
 * transaction's output that it claims and a signature that matches the
 * output's public key.
 */
class CTxIn
{
public:
    COutPoint prevout;
    CScript scriptSig;
    uint32_t nSequence;
    CScriptWitness scriptWitness; //! Only serialized through CTransaction

    /* Setting nSequence to this value for every input in a transaction
     * disables nLockTime. */
    static const uint32_t SEQUENCE_FINAL = 0xffffffff;

    /* Below flags apply in the context of BIP 68*/
    /* If this flag set, CTxIn::nSequence is NOT interpreted as a
     * relative lock-time. */
    static const uint32_t SEQUENCE_LOCKTIME_DISABLE_FLAG = (1 << 31);

    /* If CTxIn::nSequence encodes a relative lock-time and this flag
     * is set, the relative lock-time has units of 512 seconds,
     * otherwise it specifies blocks with a granularity of 1. */
    static const uint32_t SEQUENCE_LOCKTIME_TYPE_FLAG = (1 << 22);

    /* If CTxIn::nSequence encodes a relative lock-time, this mask is
     * applied to extract that lock-time from the sequence field. */
    static const uint32_t SEQUENCE_LOCKTIME_MASK = 0x0000ffff;

    /* In order to use the same number of bits to encode roughly the
     * same wall-clock duration, and because blocks are naturally
     * limited to occur every 600s on average, the minimum granularity
     * for time-based relative lock-time is fixed at 512 seconds.
     * Converting from CTxIn::nSequence to seconds is performed by
     * multiplying by 512 = 2^9, or equivalently shifting up by
     * 9 bits. */
    static const int SEQUENCE_LOCKTIME_GRANULARITY = 9;

    CTxIn()
    {
        nSequence = SEQUENCE_FINAL;
    }

    explicit CTxIn(COutPoint prevoutIn, CScript scriptSigIn=CScript(), uint32_t nSequenceIn=SEQUENCE_FINAL);
    CTxIn(uint256 hashPrevTx, uint32_t nOut, CScript scriptSigIn=CScript(), uint32_t nSequenceIn=SEQUENCE_FINAL);

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(prevout);
        READWRITE(*(CScriptBase*)(&scriptSig));
        READWRITE(nSequence);
    }

    friend bool operator==(const CTxIn& a, const CTxIn& b)
    {
        return (a.prevout   == b.prevout &&
                a.scriptSig == b.scriptSig &&
                a.nSequence == b.nSequence);
    }

    friend bool operator!=(const CTxIn& a, const CTxIn& b)
    {
        return !(a == b);
    }

    std::string ToString() const;
};

class CTxOutStandard;
class CTxOutCT;
class CTxOutRingCT;
class CTxOutData;

class CTxOutBase
{
public:
    CTxOutBase(uint8_t v) : nVersion(v) {};
    uint8_t nVersion;
    
    template<typename Stream>
    void Serialize(Stream &s) const
    {
        switch (nVersion)
        {
            case OUTPUT_STANDARD:
                s << *((CTxOutStandard*) this);
                break;
            case OUTPUT_CT:
                s << *((CTxOutCT*) this);
                break;
            case OUTPUT_RINGCT:
                s << *((CTxOutRingCT*) this);
                break;
            case OUTPUT_DATA:
                s << *((CTxOutData*) this);
                break;
            default:
                assert(false);
        };
    };
    
    template<typename Stream>
    void Unserialize(Stream &s)
    {
        switch (nVersion)
        {
            case OUTPUT_STANDARD:
                s >> *((CTxOutStandard*) this);
                break;
            case OUTPUT_CT:
                s >> *((CTxOutCT*) this);
                break;
            case OUTPUT_RINGCT:
                s >> *((CTxOutRingCT*) this);
                break;
            case OUTPUT_DATA:
                s >> *((CTxOutData*) this);
                break;
            default:
                assert(false);
        };
    };
    
    uint8_t GetType() const
    {
        return nVersion;
    };
    
    bool IsStandardOutput() const
    {
        return nVersion == OUTPUT_STANDARD;
    };
    
    const CTxOutStandard *GetStandardOutput() const
    {
        assert(nVersion == OUTPUT_STANDARD);
        return (CTxOutStandard*)this;
    };
    
    
    virtual bool IsEmpty() const { return false;}
    
    void SetValue(CAmount value);
    
    CAmount GetValue() const;
    
    std::string ToString() const;
};


typedef std::shared_ptr<CTxOutBase> CTxOutBaseRef;
#define OUTPUT_PTR std::shared_ptr
#define MAKE_OUTPUT std::make_shared

typedef OUTPUT_PTR<CTxOutBase> CTxOutBaseRef;


class CTxOutStandard : public CTxOutBase
{
public:
    CTxOutStandard() : CTxOutBase(OUTPUT_STANDARD) {};
    CTxOutStandard(const CAmount& nValueIn, CScript scriptPubKeyIn);
    
    CAmount nValue;
    CScript scriptPubKey;
    
    CAmount GetDustThreshold(const CFeeRate &minRelayTxFee) const
    {
        // "Dust" is defined in terms of CTransaction::minRelayTxFee,
        // which has units satoshis-per-kilobyte.
        // If you'd pay more than 1/3 in fees
        // to spend something, then we consider it dust.
        // A typical spendable non-segwit txout is 34 bytes big, and will
        // need a CTxIn of at least 148 bytes to spend:
        // so dust is a spendable txout less than
        // 546*minRelayTxFee/1000 (in satoshis).
        // A typical spendable segwit txout is 31 bytes big, and will
        // need a CTxIn of at least 67 bytes to spend:
        // so dust is a spendable txout less than
        // 294*minRelayTxFee/1000 (in satoshis).
        if (scriptPubKey.IsUnspendable())
            return 0;

        size_t nSize = GetSerializeSize(*this, SER_DISK, 0);
        int witnessversion = 0;
        std::vector<unsigned char> witnessprogram;

        if (scriptPubKey.IsWitnessProgram(witnessversion, witnessprogram)) {
            // sum the sizes of the parts of a transaction input
            // with 75% segwit discount applied to the script size.
            nSize += (32 + 4 + 1 + (107 / WITNESS_SCALE_FACTOR) + 4);
        } else {
            nSize += (32 + 4 + 1 + 107 + 4); // the 148 mentioned above
        }

        return 3 * minRelayTxFee.GetFee(nSize);
    }

    bool IsDust(const CFeeRate &minRelayTxFee) const
    {
        return (nValue < GetDustThreshold(minRelayTxFee));
    }
    
    virtual bool IsEmpty() const
    {
        return (nValue == 0 && scriptPubKey.empty());
    }
    
    template<typename Stream>
    void Serialize(Stream &s) const
    {
        s << nValue;
        s << *(CScriptBase*)(&scriptPubKey);
    };
    
    template<typename Stream>
    void Unserialize(Stream &s)
    {
        s >> nValue;
        s >> *(CScriptBase*)(&scriptPubKey);
    };
};

class CTxOutCT : public CTxOutBase
{
public:
    CTxOutCT() : CTxOutBase(OUTPUT_CT) {};
    secp256k1_pedersen_commitment commitment;
    std::vector<uint8_t> vData; // first 33 bytes is always ephemeral pubkey, can contain token for stealth prefix matching
    CScript scriptPubKey;
    
    std::vector<uint8_t> vRangeproof;
    
    template<typename Stream>
    void Serialize(Stream &s) const
    {
        const bool fAllowWitness = !(s.GetVersion() & SERIALIZE_TRANSACTION_NO_WITNESS);
        s.write((char*)&commitment.data[0], 33);
        s << vData;
        s << *(CScriptBase*)(&scriptPubKey);
        
        if (fAllowWitness)
            s << vRangeproof;
    };
    
    template<typename Stream>
    void Unserialize(Stream &s)
    {
        s.read((char*)&commitment.data[0], 33);
        s >> vData;
        s >> *(CScriptBase*)(&scriptPubKey);
        
        s >> vRangeproof;
    };
};

class CTxOutRingCT : public CTxOutBase
{
public:
    CTxOutRingCT() : CTxOutBase(OUTPUT_RINGCT) {};
    CPubKey pk;
    secp256k1_pedersen_commitment commitment;
    std::vector<uint8_t> vchRangeproof;
    std::vector<uint8_t> vchNonceCommitment;
    
    template<typename Stream>
    void Serialize(Stream &s) const
    {
        
    };
    
    template<typename Stream>
    void Unserialize(Stream &s)
    {
        
    };
};

class CTxOutData : public CTxOutBase
{
public:
    CTxOutData() : CTxOutBase(OUTPUT_DATA) {};
    std::vector<uint8_t> vData;
    
    template<typename Stream>
    void Serialize(Stream &s) const
    {
        s << vData;
    };
    
    template<typename Stream>
    void Unserialize(Stream &s)
    {
        s >> vData;
    };
};


/** An output of a transaction.  It contains the public key that the next input
 * must be able to sign with to claim it.
 */
class CTxOut
{
public:
    CAmount nValue;
    CScript scriptPubKey;

    CTxOut()
    {
        SetNull();
    }

    CTxOut(const CAmount& nValueIn, CScript scriptPubKeyIn);

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(nValue);
        READWRITE(*(CScriptBase*)(&scriptPubKey));
    }

    void SetNull()
    {
        nValue = -1;
        scriptPubKey.clear();
    }

    bool IsNull() const
    {
        return (nValue == -1);
    }

    bool IsEmpty() const
    {
        return (nValue == 0 && scriptPubKey.empty());
    }
    
    CAmount GetDustThreshold(const CFeeRate &minRelayTxFee) const
    {
        // "Dust" is defined in terms of CTransaction::minRelayTxFee,
        // which has units satoshis-per-kilobyte.
        // If you'd pay more than 1/3 in fees
        // to spend something, then we consider it dust.
        // A typical spendable non-segwit txout is 34 bytes big, and will
        // need a CTxIn of at least 148 bytes to spend:
        // so dust is a spendable txout less than
        // 546*minRelayTxFee/1000 (in satoshis).
        // A typical spendable segwit txout is 31 bytes big, and will
        // need a CTxIn of at least 67 bytes to spend:
        // so dust is a spendable txout less than
        // 294*minRelayTxFee/1000 (in satoshis).
        if (scriptPubKey.IsUnspendable())
            return 0;

        size_t nSize = GetSerializeSize(*this, SER_DISK, 0);
        int witnessversion = 0;
        std::vector<unsigned char> witnessprogram;

        if (scriptPubKey.IsWitnessProgram(witnessversion, witnessprogram)) {
            // sum the sizes of the parts of a transaction input
            // with 75% segwit discount applied to the script size.
            nSize += (32 + 4 + 1 + (107 / WITNESS_SCALE_FACTOR) + 4);
        } else {
            nSize += (32 + 4 + 1 + 107 + 4); // the 148 mentioned above
        }

        return 3 * minRelayTxFee.GetFee(nSize);
    }

    bool IsDust(const CFeeRate &minRelayTxFee) const
    {
        return (nValue < GetDustThreshold(minRelayTxFee));
    }

    friend bool operator==(const CTxOut& a, const CTxOut& b)
    {
        return (a.nValue       == b.nValue &&
                a.scriptPubKey == b.scriptPubKey);
    }

    friend bool operator!=(const CTxOut& a, const CTxOut& b)
    {
        return !(a == b);
    }

    std::string ToString() const;
};

struct CMutableTransaction;

/**
 * Basic transaction serialization format:
 * - int32_t nVersion
 * - std::vector<CTxIn> vin
 * - std::vector<CTxOut> vout
 * - uint32_t nLockTime
 *
 * Extended transaction serialization format:
 * - int32_t nVersion
 * - unsigned char dummy = 0x00
 * - unsigned char flags (!= 0)
 * - std::vector<CTxIn> vin
 * - std::vector<CTxOut> vout
 * - if (flags & 1):
 *   - CTxWitness wit;
 * - uint32_t nLockTime
 */
template<typename Stream, typename TxType>
inline void UnserializeTransaction(TxType& tx, Stream& s) {
    const bool fAllowWitness = !(s.GetVersion() & SERIALIZE_TRANSACTION_NO_WITNESS);

    uint8_t bv;
    tx.nVersion = 0;
    s >> bv;
    
    if (bv >= PARTICL_TXN_VERSION)
    {
        tx.nVersion = bv;
        
        s >> bv;
        tx.nVersion |= bv<<8; // TransactionTypes
        
        //s >> tx.nTime;
        tx.nLockTime = ReadCompactSize(s);
        
        tx.vin.clear();
        s >> tx.vin;
        
        size_t nOutputs = ReadCompactSize(s);
        tx.vpout.resize(nOutputs);
        for (size_t k = 0; k < tx.vpout.size(); ++k)
        {
            s >> bv;
            
            switch (bv)
            {
                case OUTPUT_STANDARD:
                    tx.vpout[k] = MAKE_OUTPUT<CTxOutStandard>();
                    break;
                case OUTPUT_CT:
                    tx.vpout[k] = MAKE_OUTPUT<CTxOutCT>();
                    break;
                case OUTPUT_RINGCT:
                    tx.vpout[k] = MAKE_OUTPUT<CTxOutRingCT>();
                    break;
                case OUTPUT_DATA:
                    tx.vpout[k] = MAKE_OUTPUT<CTxOutData>();
                    break;
                default:
                    return;
                    //assert(false);
            };
            
            tx.vpout[k]->nVersion = bv;
            s >> *tx.vpout[k];
        }
        
        for (auto &txin : tx.vin)
            s >> txin.scriptWitness.stack;
        return;
    };
    
    tx.nVersion |= bv;
    s >> bv;
    tx.nVersion |= bv<<8;
    s >> bv;
    tx.nVersion |= bv<<16;
    s >> bv;
    tx.nVersion |= bv<<24;
    
    
    unsigned char flags = 0;
    tx.vin.clear();
    tx.vout.clear();
    /* Try to read the vin. In case the dummy is there, this will be read as an empty vector. */
    s >> tx.vin;
    if (tx.vin.size() == 0 && fAllowWitness) {
        /* We read a dummy or an empty vin. */
        s >> flags;
        if (flags != 0) {
            s >> tx.vin;
            s >> tx.vout;
        }
    } else {
        /* We read a non-empty vin. Assume a normal vout follows. */
        s >> tx.vout;
    }
    if ((flags & 1) && fAllowWitness) {
        /* The witness flag is present, and we support witnesses. */
        flags ^= 1;
        for (size_t i = 0; i < tx.vin.size(); i++) {
            s >> tx.vin[i].scriptWitness.stack;
        }
    }
    if (flags) {
        /* Unknown flag in the serialization */
        throw std::ios_base::failure("Unknown transaction optional data");
    }
    s >> tx.nLockTime;
}

template<typename Stream, typename TxType>
inline void SerializeTransaction(const TxType& tx, Stream& s) {
    const bool fAllowWitness = !(s.GetVersion() & SERIALIZE_TRANSACTION_NO_WITNESS);
    
    if (IsParticlTxVersion(tx.nVersion))
    {
        uint8_t bv = tx.nVersion & 0xFF;
        s << bv;
        
        bv = (tx.nVersion>>8) & 0xFF;
        s << bv; // TransactionType
        
        //s << tx.nTime;
        WriteCompactSize(s, tx.nLockTime);
        s << tx.vin;
        
        WriteCompactSize(s, tx.vpout.size());
        for (size_t k = 0; k < tx.vpout.size(); ++k)
        {
            s << tx.vpout[k]->nVersion;
            s << *tx.vpout[k];
        };
        
        for (auto &txin : tx.vin)
            s << txin.scriptWitness.stack;
        return;
    };
    
    s << tx.nVersion;
    
    unsigned char flags = 0;
    // Consistency check
    if (fAllowWitness) {
        /* Check whether witnesses need to be serialized. */
        if (tx.HasWitness()) {
            flags |= 1;
        }
    }
    if (flags) {
        /* Use extended format in case witnesses are to be serialized. */
        std::vector<CTxIn> vinDummy;
        s << vinDummy;
        s << flags;
    }
    s << tx.vin;
    s << tx.vout;
    if (flags & 1) {
        for (size_t i = 0; i < tx.vin.size(); i++) {
            s << tx.vin[i].scriptWitness.stack;
        }
    }
    s << tx.nLockTime;
}


/** The basic transaction that is broadcasted on the network and contained in
 * blocks.  A transaction can contain multiple inputs and outputs.
 */
class CTransaction
{
public:
    // Default transaction version.
    static const int32_t CURRENT_VERSION=2;

    // Changing the default transaction version requires a two step process: first
    // adapting relay policy by bumping MAX_STANDARD_VERSION, and then later date
    // bumping the default CURRENT_VERSION at which point both CURRENT_VERSION and
    // MAX_STANDARD_VERSION will be equal.
    static const int32_t MAX_STANDARD_VERSION=2;
    
    //static const int32_t CURRENT_PARTICL_VERSION=2;

    // The local variables are made const to prevent unintended modification
    // without updating the cached hash value. However, CTransaction is not
    // actually immutable; deserialization and assignment are implemented,
    // and bypass the constness. This is safe, as they update the entire
    // structure, including the hash.
    const int32_t nVersion;
    const std::vector<CTxIn> vin;
    const std::vector<CTxOut> vout;
    const std::vector<CTxOutBaseRef> vpout;
    const uint32_t nLockTime;

private:
    /** Memory only. */
    const uint256 hash;

    uint256 ComputeHash() const;

public:
    /** Construct a CTransaction that qualifies as IsNull() */
    CTransaction();
    ~CTransaction() {};

    /** Convert a CMutableTransaction into a CTransaction. */
    CTransaction(const CMutableTransaction &tx);
    CTransaction(CMutableTransaction &&tx);

    template <typename Stream>
    inline void Serialize(Stream& s) const {
        SerializeTransaction(*this, s);
    }

    /** This deserializing constructor is provided instead of an Unserialize method.
     *  Unserialize is not possible, since it would require overwriting const fields. */
    template <typename Stream>
    CTransaction(deserialize_type, Stream& s) : CTransaction(CMutableTransaction(deserialize, s)) {}

    bool IsNull() const {
        return vin.empty() && vout.empty() && vpout.empty();
    }
    
    bool IsParticlVersion() const {
        return IsParticlTxVersion(nVersion);
    }
    
    int GetType() const {
        return (nVersion >> 8) & 0xFF;
    }
    
    const uint256& GetHash() const {
        return hash;
    }

    // Compute a hash that includes both transaction and witness data
    uint256 GetWitnessHash() const;

    // Return sum of txouts.
    CAmount GetValueOut() const;
    // GetValueIn() is a method on CCoinsViewCache, because
    // inputs must be known to compute value in.

    // Compute priority, given priority of inputs and (optionally) tx size
    double ComputePriority(double dPriorityInputs, unsigned int nTxSize=0) const;

    // Compute modified tx size for priority calculation (optionally given tx size)
    unsigned int CalculateModifiedSize(unsigned int nTxSize=0) const;

    /**
     * Get the total transaction size in bytes, including witness data.
     * "Total Size" defined in BIP141 and BIP144.
     * @return Total transaction size in bytes
     */
    unsigned int GetTotalSize() const;

    bool IsCoinBase() const
    {
        if (IsParticlVersion())
            return (GetType() == TXN_COINBASE
                && vin.size() == 1 && vin[0].prevout.IsNull()); // TODO [rm]?
        
        return (vin.size() == 1 && vin[0].prevout.IsNull());
    }

    bool IsCoinStake() const
    {
        return GetType() == TXN_COINSTAKE
            && vin.size() > 0 && vpout.size() > 0;
    }

    bool GetCTFee(CAmount &nFee) const
    {
        if (vpout.size() < 2 || vpout[0]->nVersion != OUTPUT_DATA)
            return false;
        
        std::vector<uint8_t> &vData = ((CTxOutData*)vpout[0].get())->vData;
        if (vData.size() != 9) // < ?
            return false;
        
        memcpy(&nFee, &vData[1], 8);
        return true;
    }

    friend bool operator==(const CTransaction& a, const CTransaction& b)
    {
        return a.hash == b.hash;
    }

    friend bool operator!=(const CTransaction& a, const CTransaction& b)
    {
        return a.hash != b.hash;
    }

    std::string ToString() const;

    bool HasWitness() const
    {
        for (size_t i = 0; i < vin.size(); i++) {
            if (!vin[i].scriptWitness.IsNull()) {
                return true;
            }
        }
        return false;
    }
};

/** A mutable version of CTransaction. */
struct CMutableTransaction
{
    int32_t nVersion;
    std::vector<CTxIn> vin;
    std::vector<CTxOut> vout;
    std::vector<CTxOutBaseRef> vpout;
    uint32_t nLockTime;

    CMutableTransaction();
    CMutableTransaction(const CTransaction& tx);

    template <typename Stream>
    inline void Serialize(Stream& s) const {
        SerializeTransaction(*this, s);
    }


    template <typename Stream>
    inline void Unserialize(Stream& s) {
        UnserializeTransaction(*this, s);
    }

    template <typename Stream>
    CMutableTransaction(deserialize_type, Stream& s) {
        Unserialize(s);
    }
    
    void SetType(int type) {
        nVersion |= (type & 0xFF) << 8;
    }
    
    bool IsParticlVersion() const {
        return IsParticlTxVersion(nVersion);
    }

    /** Compute the hash of this CMutableTransaction. This is computed on the
     * fly, as opposed to GetHash() in CTransaction, which uses a cached result.
     */
    uint256 GetHash() const;

    friend bool operator==(const CMutableTransaction& a, const CMutableTransaction& b)
    {
        return a.GetHash() == b.GetHash();
    }

    bool HasWitness() const
    {
        for (size_t i = 0; i < vin.size(); i++) {
            if (!vin[i].scriptWitness.IsNull()) {
                return true;
            }
        }
        return false;
    }
};

typedef std::shared_ptr<const CTransaction> CTransactionRef;
static inline CTransactionRef MakeTransactionRef() { return std::make_shared<const CTransaction>(); }
template <typename Tx> static inline CTransactionRef MakeTransactionRef(Tx&& txIn) { return std::make_shared<const CTransaction>(std::forward<Tx>(txIn)); }

/** Compute the weight of a transaction, as defined by BIP 141 */
int64_t GetTransactionWeight(const CTransaction &tx);

#endif // BITCOIN_PRIMITIVES_TRANSACTION_H
