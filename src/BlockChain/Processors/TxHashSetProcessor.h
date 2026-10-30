#pragma once

#include "../ChainState.h"

#include <PMMR/TxHashSet.h>
#include <Core/Config.h>
#include <Crypto/Models/Hash.h>
#include <P2P/SyncStatus.h>
#include <filesystem.h>
#include <string>

// Forward Declarations
class IBlockChain;
class BlockHeader;

class TxHashSetProcessor
{
public:
	TxHashSetProcessor(const Config& config, IBlockChain& blockChain, std::shared_ptr<Locked<ChainState>> pChainState);

	bool ProcessTxHashSet(const Hash& blockHash, const fs::path& path, SyncStatus& syncStatus);

	// Validates and installs a txhashset that is already on disk at the configured
	// path (PIBD writes and activates the files before calling this).
	bool ProcessExistingTxHashSet(const Hash& blockHash, SyncStatus& syncStatus);

private:
	bool UpdateConfirmedChain(Writer<ChainState> pLockedState, const BlockHeader& blockHeader);

	const Config& m_config;
	IBlockChain& m_blockChain;
	std::shared_ptr<Locked<ChainState>> m_pChainState;
};