// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pow.h"
#include "arith_uint256.h"
#include "chain.h"
#include "chainparams.h"
#include "primitives/block.h"
#include "uint256.h"
#include "util.h"

#include <math.h>

unsigned int Terminal_Velocity_RateX(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    // Terminal-Velocity-RateX, v10-Beta-R7.2 (Pure PoW), written by Jonathan Dan Zaretsky - cryptocoderz@gmail.com
    const arith_uint256 bnTerminalVelocity = UintToArith256(params.powLimit);
    // Define values
    double VLF1 = 0;
    double VLF2 = 0;
    double VLF3 = 0;
    double VLF4 = 0;
    double VLF5 = 0;
    double VLFtmp = 0;
    double VRFsm1 = 1;
    double VRFdw1 = 0.75;
    double VRFdw2 = 0.5;
    double VRFup1 = 1.25;
    double VRFup2 = 1.5;
    double VRFup3 = 2;
    double TerminalAverage = 0;
    double TerminalFactor = 10000;
    int64_t VLrate1 = 0;
    int64_t VLrate2 = 0;
    int64_t VLrate3 = 0;
    int64_t VLrate4 = 0;
    int64_t VLrate5 = 0;
    int64_t VLRtemp = 0;
    int64_t DSrateNRM = params.nPowTargetSpacing;
    int64_t DSrateMAX = DSrateNRM + (1 * 60);
    int64_t FRrateCLNG = DSrateNRM + (3 * 60);
    int64_t FRrateDWN = DSrateNRM - (1 * 60);
    int64_t FRrateFLR = DSrateNRM - (2 * 60);
    int64_t difficultyfactor = 0;
    int64_t AverageDivisor = 5;
    int64_t scanheight = 6;
    int64_t scanblocks = 1;
    int64_t scantime_1 = 0;
    int64_t scantime_2 = pindexLast->GetBlockTime();
    // Check for blocks to index | Allowing for diff reset
    if (pindexLast->nHeight < params.nPowVRXHeight+2)
        return bnTerminalVelocity.GetCompact(); // reset diff
    // Check for chain stall, allowing for min diff reset
    // If the new block's timestamp is more than 2 * target spacing
    // then allow mining of a min-difficulty block.
    if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + (DSrateNRM * 2)) {
        // Min-diff activation after block xxxxxxx
        if (pindexLast->nHeight > params.nMinDiffVRXHeight) {
            return bnTerminalVelocity.GetCompact(); // reset diff
        }
    }
    // Set prev blocks...
    const CBlockIndex* pindexPrev = pindexLast;
    // ...and deduce spacing
    while(scanblocks < scanheight)
    {
        scantime_1 = scantime_2;
        pindexPrev = pindexPrev->pprev;
        scantime_2 = pindexPrev->GetBlockTime();
        // Set standard values
        if(scanblocks > 0){
            if     (scanblocks < scanheight-4){ VLrate1 = (scantime_1 - scantime_2); VLRtemp = VLrate1; }
            else if(scanblocks < scanheight-3){ VLrate2 = (scantime_1 - scantime_2); VLRtemp = VLrate2; }
            else if(scanblocks < scanheight-2){ VLrate3 = (scantime_1 - scantime_2); VLRtemp = VLrate3; }
            else if(scanblocks < scanheight-1){ VLrate4 = (scantime_1 - scantime_2); VLRtemp = VLrate4; }
            else if(scanblocks < scanheight-0){ VLrate5 = (scantime_1 - scantime_2); VLRtemp = VLrate5; }
        }
        // Round factoring
        if(VLRtemp >= DSrateNRM){ VLFtmp = VRFsm1;
            if(VLRtemp > DSrateMAX){ VLFtmp = VRFdw1;
                if(VLRtemp > FRrateCLNG){ VLFtmp = VRFdw2; }
            }
        }
        else if(VLRtemp < DSrateNRM){ VLFtmp = VRFup1;
            if(VLRtemp < FRrateDWN){ VLFtmp = VRFup2;
                if(VLRtemp < FRrateFLR){ VLFtmp = VRFup3; }
            }
        }
        // Record factoring
        if      (scanblocks < scanheight-4) VLF1 = VLFtmp;
        else if (scanblocks < scanheight-3) VLF2 = VLFtmp;
        else if (scanblocks < scanheight-2) VLF3 = VLFtmp;
        else if (scanblocks < scanheight-1) VLF4 = VLFtmp;
        else if (scanblocks < scanheight-0) VLF5 = VLFtmp;
        // move up per scan round
        scanblocks ++;
    }
    // Final mathematics
    TerminalAverage = (VLF1 + VLF2 + VLF3 + VLF4 + VLF5) / AverageDivisor;
    // Only select last non-min-diff block when retargeting
    // This negates a chain reset when a min-diff block is allowed
    //
    // Set min-diff check values
    arith_uint256 bnNonMinDiff;
    const CBlockIndex* pindexNonMinDiff = pindexLast;
    bnNonMinDiff.SetCompact(pindexNonMinDiff->nBits);
    // Min-diff index skip after block xxxxxxx
    if (pindexLast->nHeight > params.nMinDiffVRXHeight) {
        // Check whether the selected block is min-diff
        while(bnNonMinDiff.GetCompact() <= bnTerminalVelocity.GetCompact()) {
            // Index backwards until a non-min-diff block is found
            pindexNonMinDiff = pindexNonMinDiff->pprev;
            bnNonMinDiff.SetCompact(pindexNonMinDiff->nBits);
        }
    }
    // Log PoW prev block
    const CBlockIndex* BlockVelocityType = pindexNonMinDiff;
    // Retarget
    arith_uint256 bnOld;
    arith_uint256 bnNew;
    TerminalFactor *= TerminalAverage;
    difficultyfactor = TerminalFactor;
    bnOld.SetCompact(BlockVelocityType->nBits);
    bnNew = bnOld / difficultyfactor;
    bnNew *= 10000;
    // Limit
    if (bnNew > bnTerminalVelocity)
        bnNew = bnTerminalVelocity;
    // Print for debugging
    if(fDebug) {
        LogPrintf("Terminal-Velocity 1st spacing: %u: \n",VLrate1);
        LogPrintf("Terminal-Velocity 2nd spacing: %u: \n",VLrate2);
        LogPrintf("Terminal-Velocity 3rd spacing: %u: \n",VLrate3);
        LogPrintf("Terminal-Velocity 4th spacing: %u: \n",VLrate4);
        LogPrintf("Terminal-Velocity 5th spacing: %u: \n",VLrate5);
        LogPrintf("Desired normal spacing: %u: \n",DSrateNRM);
        LogPrintf("Desired maximum spacing: %u: \n",DSrateMAX);
        LogPrintf("Terminal-Velocity 1st multiplier set to: %f: \n",VLF1);
        LogPrintf("Terminal-Velocity 2nd multiplier set to: %f: \n",VLF2);
        LogPrintf("Terminal-Velocity 3rd multiplier set to: %f: \n",VLF3);
        LogPrintf("Terminal-Velocity 4th multiplier set to: %f: \n",VLF4);
        LogPrintf("Terminal-Velocity 5th multiplier set to: %f: \n",VLF5);
        LogPrintf("Terminal-Velocity averaged a final multiplier of: %f: \n",TerminalAverage);
        LogPrintf("Prior Terminal-Velocity: %08x  %s\n", BlockVelocityType->nBits, bnOld.ToString());
        LogPrintf("New Terminal-Velocity:  %08x  %s\n", bnNew.GetCompact(), bnNew.ToString());
    }
    return bnNew.GetCompact();
}

unsigned int GetNextWorkRequiredBTC(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    unsigned int nProofOfWorkLimit = UintToArith256(params.powLimit).GetCompact();

    // Genesis block
    if (pindexLast == NULL)
        return nProofOfWorkLimit;

    // Only change once per interval
    if ((pindexLast->nHeight+1) % params.DifficultyAdjustmentInterval() != 0)
    {
        if (params.fPowAllowMinDifficultyBlocks)
        {
            // Special difficulty rule for testnet:
            // If the new block's timestamp is more than 2* 2.5 minutes
            // then allow mining of a min-difficulty block.
            if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + params.nPowTargetSpacing*2)
                return nProofOfWorkLimit;
            else
            {
                // Return the last non-special-min-difficulty-rules-block
                const CBlockIndex* pindex = pindexLast;
                while (pindex->pprev && pindex->nHeight % params.DifficultyAdjustmentInterval() != 0 && pindex->nBits == nProofOfWorkLimit)
                    pindex = pindex->pprev;
                return pindex->nBits;
            }
        }
        return pindexLast->nBits;
    }

    // Go back by what we want to be 1 day worth of blocks
    int nHeightFirst = pindexLast->nHeight - (params.DifficultyAdjustmentInterval()-1);
    assert(nHeightFirst >= 0);
    const CBlockIndex* pindexFirst = pindexLast->GetAncestor(nHeightFirst);
    assert(pindexFirst);

   return CalculateNextWorkRequired(pindexLast, pindexFirst->GetBlockTime(), params);
}

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    // Most recent algo first
    if (pindexLast->nHeight+1 >= params.nPowVRXHeight) {
        // Print for debugging
        if(fDebug)
            LogPrintf("Retargeting using VRX retarget logic \n");

        return Terminal_Velocity_RateX(pindexLast, pblock, params);
    }
    else {
        // Print for debugging
        if(fDebug)
            LogPrintf("Retargeting using BTC retarget logic \n");

        return GetNextWorkRequiredBTC(pindexLast, pblock, params);
    }
}

// for DIFF_BTC only!
unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params& params)
{
    if (params.fPowNoRetargeting)
        return pindexLast->nBits;

    // Limit adjustment step
    int64_t nActualTimespan = pindexLast->GetBlockTime() - nFirstBlockTime;
    if (nActualTimespan < params.nPowTargetTimespan/4)
        nActualTimespan = params.nPowTargetTimespan/4;
    if (nActualTimespan > params.nPowTargetTimespan*4)
        nActualTimespan = params.nPowTargetTimespan*4;

    // Retarget
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    bnNew *= nActualTimespan;
    bnNew /= params.nPowTargetTimespan;

    if (bnNew > bnPowLimit)
        bnNew = bnPowLimit;

    return bnNew.GetCompact();
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powLimit))
        return false;

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return false;

    return true;
}
