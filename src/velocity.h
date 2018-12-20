// Copyright (c) 2014 The Cryptocoin Revival Foundation
// Copyright (c) 2015-2018 The CryptoCoderz Team / Espers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef VELOCITY_H
#define VELOCITY_H 1

#include "chain.h"
#include "chainparams.h"
#include "validation.h"

class CBlock;
class CBlockIndex;

static const          int VELOCITY_HEIGHT[]    = { 6810 }; /** Height to start Velocity */
static const          int VELOCITY_TERMINAL[]  = { 6800 }; /** Height to start Velocity retargetting */
static const          int VELOCITY_MAX_RATE[]  = { (1 * 8 * 60) + (3 * 60) }; /** Rate to Velocity in seconds */
static const          int VELOCITY_RATE[]      = { (1 * 8 * 60) }; /** Rate to Velocity in seconds */
static const          int VELOCITY_MIN_RATE[]  = { (1 * 8 * 60) - (3 * 60) }; /** Rate to Velocity in seconds */
static const unsigned int VELOCITY_MIN_TX[]    = { 0 }; /** Minimum amount (not value of!) of TX in a block to bypass Velocity-Rate */
static const          int VELOCITY_MIN_VALUE[] = { 0 }; /** Minimum value of the TX in a block to bypass Velocity-Rate (without COIN base) */
static const          int VELOCITY_MIN_FEE[]   = { 0 }; /** Minimum value of accumulated fees of the TX in a block to bypass Velocity-Rate (without COIN base) */
static const         bool VELOCITY_FACTOR[]    = { false }; /** Treat Switches as factors of BlockReward */
static const         bool VELOCITY_EXPLICIT[]  = { false }; /** Require all switches to trigger a block */

bool Velocity_check(int nHeight);
bool Velocity(CBlockIndex* prevBlock, const CBlock& block);

int VelocityI(int nHeight);

#endif
