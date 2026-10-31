#include "BlockChainImpl.h"
#include "BlockHydrator.h"
#include "CompactBlockFactory.h"
#include "Validators/BlockHeaderValidator.h"
#include "Validators/BlockValidator.h"
#include "Processors/BlockHeaderProcessor.h"
#include "Processors/TxHashSetProcessor.h"
#include "Processors/BlockProcessor.h"
#include "ChainResyncer.h"
#include "CoinView.h"
#include "ChainConsistency.h"
#include "DbCheckpoint.h"
#include <BlockChain/DbRecovery.h>
#include <PMMR/Common/MmrStructure.h>

#include <Consensus.h>
#include <GrinVersion.h>
#include <Common/Logger.h>
#include <Core/Global.h>
#include <Core/Exceptions/BadDataException.h>
#include <Core/Config.h>
#include <PMMR/TxHashSet.h>
#include <filesystem.h>
#include <algorithm>

BlockChain::BlockChain(
	std::shared_ptr<ITransactionPool> pTransactionPool,
	std::shared_ptr<Locked<ChainState>> pChainState)
	: m_pTransactionPool(pTransactionPool), m_pChainState(pChainState)
{

}

BlockChain::~BlockChain()
{
	Global::SetCoinView(nullptr);
	m_pChainState.reset();
	m_pTransactionPool.reset();
}

std::shared_ptr<BlockChain> BlockChain::Create(
	const Config& config,
	std::shared_ptr<Locked<IBlockDB>> pDatabase,
	std::shared_ptr<Locked<TxHashSetManager>> pTxHashSetManager,
	std::shared_ptr<ITransactionPool> pTransactionPool,
	std::shared_ptr<Locked<IHeaderMMR>> pHeaderMMR)
{
	const FullBlock& genesisBlock = Global::GetGenesisBlock();
	auto pGenesisIndex = std::make_shared<BlockIndex>(genesisBlock.GetHash(), 0);

	auto pChainStore = ChainStore::Load(config, pGenesisIndex);

	// header_mmr.bin and the chain files are committed independently. A crash
	// between the commits leaves them at different heights; loading such a state
	// used to read a header beyond the MMR. Repair the stores before anything
	// else opens them (GAP G-23/G-48).
	{
		// A raw node count that cannot describe any MMR means the append-only
		// file was truncated mid-write. Cut it back to the largest valid size
		// before any height is derived from it.
		const uint64_t rawMmrNodes = pHeaderMMR->Read()->GetNodeCount();
		if (!MmrStructure::IsValidNodeCount(rawMmrNodes)) {
			const uint64_t validNodes = MmrStructure::LargestValidNodeCount(rawMmrNodes);
			const uint64_t validLeaves = MmrStructure::LeavesForNodes(validNodes);
			LOG_WARNING_F(
				"Header MMR has {} nodes, which is not a valid MMR size; repairing to {} nodes ({} leaves).",
				rawMmrNodes,
				validNodes,
				validLeaves);

			DbRecovery::CopyForRecovery(
				config.GetChainPath(),
				{ "header_mmr.bin", "candidate.chain", "confirmed.chain", "chain.checkpoint" });
			DbRecovery::RecordRepair();

			// Never truncate when the inverse mapping failed: a wrong structural
			// verdict must not destroy a healthy chain (regression caught live).
			if (validNodes > 0 && validLeaves > 0) {
				auto pMmrWriter = pHeaderMMR->Write();
				pMmrWriter->Rewind(validLeaves);
				pMmrWriter->Commit();
			} else if (rawMmrNodes > 0) {
				LOG_ERROR("Refusing to truncate the header MMR: could not derive a valid leaf count.");
			}
		}

		const uint64_t headerMmrLeaves = pHeaderMMR->Read()->GetLeafCount();
		const uint64_t candidateEntries =
			pChainStore->Read()->GetCandidateChain()->GetHeight() + 1;
		const uint64_t confirmedEntries =
			pChainStore->Read()->GetConfirmedChain()->GetHeight() + 1;
		const fs::path checkpointPath = config.GetChainPath() / "chain.checkpoint";
		DbCheckpointData checkpoint;
		const bool hasCheckpoint = DbCheckpoint::Read(checkpointPath, checkpoint);
		if (hasCheckpoint) {
			LOG_TRACE_F("DB checkpoint found ({}).", DbCheckpoint::Describe(checkpoint));
		}

		const ChainRepairPlan repair = hasCheckpoint
			? ComputeChainRepairToCheckpoint(
				headerMmrLeaves,
				candidateEntries,
				confirmedEntries,
				checkpoint.MmrLeaves,
				checkpoint.CandidateEntries,
				checkpoint.ConfirmedEntries)
			: ComputeChainRepair(headerMmrLeaves, candidateEntries, confirmedEntries);
		if (!repair.IsConsistent()) {
			DbRecovery::CopyForRecovery(
				config.GetChainPath(),
				{ "header_mmr.bin", "candidate.chain", "confirmed.chain", "chain.checkpoint" });
			DbRecovery::RecordRepair();

			LOG_WARNING_F(
				"Chain stores inconsistent (header_mmr leaves {}, candidate {}, confirmed {}) after an unclean shutdown; repairing to mmr {}, candidate {}, confirmed {} entries{}.",
				headerMmrLeaves,
				candidateEntries,
				confirmedEntries,
				repair.TargetMmrEntries,
				repair.TargetCandidateEntries,
				repair.TargetConfirmedEntries,
				hasCheckpoint ? " using the checkpoint" : "");

			if (repair.RewindHeaderMmr) {
				auto pMmrWriter = pHeaderMMR->Write();
				pMmrWriter->Rewind(repair.TargetMmrEntries);
				pMmrWriter->Commit();
			}

			if (repair.RewindCandidate || repair.RewindConfirmed) {
				auto pChainStoreWriter = pChainStore->Write();
				if (repair.RewindCandidate) {
					pChainStoreWriter->GetCandidateChain()->Rewind(repair.TargetCandidateEntries - 1);
				}
				if (repair.RewindConfirmed &&
					pChainStoreWriter->GetConfirmedChain()->GetHeight() + 1 > repair.TargetConfirmedEntries) {
					pChainStoreWriter->GetConfirmedChain()->Rewind(repair.TargetConfirmedEntries - 1);
				}
				pChainStoreWriter->Commit();
			}

			LOG_WARNING("Chain stores repaired.");
		}

		// Establish a fresh checkpoint for the state we are about to open. It is
		// written only after the stores above are consistent, so a later crash
		// can always fall back to it.
		DbCheckpointData fresh;
		fresh.Sequence = (hasCheckpoint ? checkpoint.Sequence : 0) + 1;
		fresh.MmrLeaves = pHeaderMMR->Read()->GetLeafCount();
		fresh.CandidateEntries = pChainStore->Read()->GetCandidateChain()->GetHeight() + 1;
		fresh.ConfirmedEntries = pChainStore->Read()->GetConfirmedChain()->GetHeight() + 1;
		if (!DbCheckpoint::Write(checkpointPath, fresh)) {
			LOG_WARNING("Failed to write the DB checkpoint.");
		}
	}

	auto pChainState = ChainState::Create(
		config,
		pChainStore,
		pDatabase,
		pHeaderMMR,
		pTransactionPool,
		pTxHashSetManager,
		genesisBlock
	);
	Global::SetCoinView(std::make_shared<CoinView>(*pChainState));

	// Migrate database
	const uint8_t db_version = pDatabase->Read()->GetVersion();
	if (db_version > 3) {
		LOG_ERROR("Database is from a newer version of Grin++!");
		std::terminate();
	}

	if (db_version < 2) {
		LOG_WARNING_F("Updating chain for version {}", GRINPP_VERSION);
		pDatabase->Write()->ClearOutputPositions();

		auto pBlockDB = pDatabase->Write();
		auto pTxHashSetReader = pTxHashSetManager->Read();
		if (pTxHashSetReader->GetTxHashSet() != nullptr) {
			pTxHashSetReader->GetTxHashSet()->SaveOutputPositions(
				pChainStore->Read()->GetCandidateChain(),
				pBlockDB.GetShared()
			);
		}
		pBlockDB->SetVersion(2);
	}

	if (db_version < 3) {
		LOG_WARNING_F("Migrating blocks to v3 for Grin++ {}", GRINPP_VERSION);
		pDatabase->Write()->MigrateBlocks();
		pDatabase->Write()->SetVersion(3);
	}

	// Trigger Compaction
	{
		auto pBatchDB = pDatabase->BatchWrite();
		pBatchDB->Compact(pChainStore->Read()->GetConfirmedChain());
		pBatchDB->Commit();

		auto pBatchTxHashSet = pTxHashSetManager->BatchWrite();
		auto pTxHashSet = pBatchTxHashSet->GetTxHashSet();
		if (pTxHashSet != nullptr) {
			const uint64_t horizon = Consensus::GetHorizonHeight(pChainState->Read()->GetHeight(EChainType::CONFIRMED));
			if (pTxHashSet->GetFlushedBlockHeader()->GetHeight() < horizon) {
				pTxHashSetManager->Write()->Close();
			} else {
				pTxHashSet->Compact();
			}

			pBatchTxHashSet->Commit();
		}
	}

	return std::shared_ptr<BlockChain>(new BlockChain(
		pTransactionPool,
		pChainState
	));
}

void BlockChain::ResyncChain()
{
	ChainResyncer(m_pChainState).ResyncChain();
}

void BlockChain::UpdateSyncStatus(SyncStatus& syncStatus) const
{
	m_pChainState->Read()->UpdateSyncStatus(syncStatus);
}

uint64_t BlockChain::GetHeight(const EChainType chainType) const
{
	return m_pChainState->Read()->GetHeight(chainType);
}

uint64_t BlockChain::GetTotalDifficulty(const EChainType chainType) const
{
	return m_pChainState->Read()->GetTotalDifficulty(chainType);
}

EBlockChainStatus BlockChain::AddBlock(const FullBlock& block)
{
	return BlockProcessor(m_pChainState).ProcessBlock(block);
}

EBlockChainStatus BlockChain::AddCompactBlock(const CompactBlock& compactBlock)
{
	const Hash& hash = compactBlock.GetHash();
	const uint64_t height = compactBlock.GetHeight();

	{
		auto pReader = m_pChainState->Read();
		auto pConfirmed = pReader->GetChainStore()->GetConfirmedChain()->GetByHeight(height);
		if (pConfirmed != nullptr && pConfirmed->GetHash() == hash)
		{
			return EBlockChainStatus::ALREADY_EXISTS;
		}
	}

	try
	{
		std::unique_ptr<FullBlock> pHydratedBlock = BlockHydrator(m_pTransactionPool).Hydrate(compactBlock);
		if (pHydratedBlock != nullptr)
		{
			const EBlockChainStatus added = AddBlock(*pHydratedBlock);
			if (added == EBlockChainStatus::INVALID)
			{
				return EBlockChainStatus::TRANSACTIONS_MISSING;
			}

			return added;
		}
	}
	catch (std::exception& e)
	{
		LOG_WARNING_F("Exception thrown: {}", e.what());
	}

	return EBlockChainStatus::TRANSACTIONS_MISSING;
}

fs::path BlockChain::SnapshotTxHashSet(BlockHeaderPtr pBlockHeader)
{
	// TODO: Use reader if possible
	auto pBatch = m_pChainState->BatchWrite(); // DO NOT COMMIT THIS BATCH
	const uint64_t horizon = Consensus::GetHorizonHeight(pBatch->GetHeight(EChainType::CONFIRMED));
	if (pBlockHeader->GetHeight() < horizon)
	{
		throw BAD_DATA_EXCEPTION(EBanReason::Abusive, "TxHashSet snapshot requested beyond horizon.");
	}

	return pBatch->GetTxHashSetManager()->SaveSnapshot(pBatch->GetBlockDB(), pBlockHeader);
}

EBlockChainStatus BlockChain::ProcessTransactionHashSet(const Hash& blockHash, const fs::path& path, SyncStatus& syncStatus)
{
	try
	{
		const bool success = TxHashSetProcessor(Global::GetConfig(), *this, m_pChainState).ProcessTxHashSet(blockHash, path, syncStatus);
		if (success)
		{
			return EBlockChainStatus::SUCCESS;
		}
	}
	catch (std::exception& e)
	{
		LOG_ERROR_F("Failed to process TxHashSet: {}", e.what());
	}

	return EBlockChainStatus::INVALID;
}

std::shared_ptr<ITxHashSet> BlockChain::GetTxHashSet()
{
	return m_pChainState->Write()->GetTxHashSetManager()->GetTxHashSet();
}
void BlockChain::CloseTxHashSet()
{
	m_pChainState->Write()->GetTxHashSetManager()->Close();
}

void BlockChain::OpenTxHashSet()
{
	const BlockHeaderPtr pTip = GetTipBlockHeader(EChainType::CONFIRMED);
	if (pTip != nullptr) {
		m_pChainState->Write()->GetTxHashSetManager()->Open(pTip);
	}
}
EBlockChainStatus BlockChain::ProcessExistingTxHashSet(const Hash& blockHash, SyncStatus& syncStatus)
{
	try {
		const bool success = TxHashSetProcessor(Global::GetConfig(), *this, m_pChainState)
			.ProcessExistingTxHashSet(blockHash, syncStatus);
		if (success) {
			return EBlockChainStatus::SUCCESS;
		}
	}
	catch (std::exception& e) {
		LOG_ERROR_F("Failed to process the existing TxHashSet: {}", e.what());
	}

	return EBlockChainStatus::INVALID;
}
EBlockChainStatus BlockChain::AddTransaction(TransactionPtr pTransaction, const EPoolType poolType)
{
	try
	{
		auto pReader = m_pChainState->Read();
		auto pLastConfimedHeader = pReader->GetTipBlockHeader(EChainType::CONFIRMED);
		if (pLastConfimedHeader != nullptr)
		{
			const EAddTransactionStatus status = m_pTransactionPool->AddTransaction(
				pReader->GetBlockDB().GetShared(),
				pReader->GetTxHashSetManager()->GetTxHashSet(),
				pTransaction,
				poolType,
				*pLastConfimedHeader
			);
			if (status == EAddTransactionStatus::ADDED)
			{
				return EBlockChainStatus::SUCCESS;
			}
			else if (status == EAddTransactionStatus::TX_INVALID)
			{
				return EBlockChainStatus::INVALID;
			}
			else
			{
				return EBlockChainStatus::UNKNOWN_ERROR;
			}
		}
	}
	catch (std::exception& e)
	{
		LOG_ERROR_F("Exception thrown: {}", e.what());
	}

	return EBlockChainStatus::INVALID;
}

TransactionPtr BlockChain::GetTransactionByKernelHash(const Hash& kernelHash) const
{
	return m_pTransactionPool->FindTransactionByKernelHash(kernelHash);
}

EBlockChainStatus BlockChain::AddBlockHeader(BlockHeaderPtr pBlockHeader)
{
	try
	{
		return BlockHeaderProcessor(m_pChainState).ProcessSingleHeader(pBlockHeader);
	}
	catch (BadDataException& e)
	{
		throw e;
	}
	catch (std::exception& e)
	{
		LOG_ERROR_F("Invalid header {} - Exception: ", *pBlockHeader, e.what());
		return EBlockChainStatus::INVALID;
	}
}

EBlockChainStatus BlockChain::AddBlockHeaders(const std::vector<BlockHeaderPtr>& blockHeaders)
{
	try
	{
		return BlockHeaderProcessor(m_pChainState).ProcessSyncHeaders(blockHeaders);
	}
	catch (BadDataException& e)
	{
		throw e;
	}
	catch (std::exception&)
	{
		return EBlockChainStatus::UNKNOWN_ERROR;
	}
}

std::vector<BlockHeaderPtr> BlockChain::GetBlockHeadersByHash(const std::vector<CBigInteger<32>>& hashes) const
{
	auto pReader = m_pChainState->Read();

	std::vector<BlockHeaderPtr> headers;
	for (const CBigInteger<32>& hash : hashes)
	{
		BlockHeaderPtr pHeader = pReader->GetBlockHeaderByHash(hash);
		if (pHeader != nullptr)
		{
			headers.push_back(pHeader);
		}
	}

	return headers;
}

BlockHeaderPtr BlockChain::GetBlockHeaderByHeight(const uint64_t height, const EChainType chainType) const
{
	return m_pChainState->Read()->GetBlockHeaderByHeight(height, chainType);
}

BlockHeaderPtr BlockChain::GetBlockHeaderByHash(const CBigInteger<32>& hash) const
{
	return m_pChainState->Read()->GetBlockHeaderByHash(hash);
}

BlockHeaderPtr BlockChain::GetBlockHeaderByCommitment(const Commitment& outputCommitment) const
{
	return m_pChainState->Read()->GetBlockHeaderByCommitment(outputCommitment);
}

BlockHeaderPtr BlockChain::GetTipBlockHeader(const EChainType chainType) const
{
	return m_pChainState->Read()->GetTipBlockHeader(chainType);
}

std::unique_ptr<CompactBlock> BlockChain::GetCompactBlockByHash(const Hash& hash) const
{
	std::unique_ptr<FullBlock> pBlock = m_pChainState->Read()->GetBlockByHash(hash);
	if (pBlock != nullptr)
	{
		return std::make_unique<CompactBlock>(CompactBlockFactory::CreateCompactBlock(*pBlock));
	}

	return std::unique_ptr<CompactBlock>(nullptr);
}

std::unique_ptr<FullBlock> BlockChain::GetBlockByCommitment(const Commitment& outputCommitment) const
{
	auto pHeader = m_pChainState->Read()->GetBlockHeaderByCommitment(outputCommitment);
	if (pHeader != nullptr)
	{
		return GetBlockByHash(pHeader->GetHash());
	}

	return std::unique_ptr<FullBlock>(nullptr);
}

std::unique_ptr<FullBlock> BlockChain::GetBlockByHash(const Hash& hash) const
{
	return m_pChainState->Read()->GetBlockByHash(hash);
}

std::unique_ptr<FullBlock> BlockChain::GetBlockByHeight(const uint64_t height) const
{
	return m_pChainState->Read()->GetBlockByHeight(height);
}

std::vector<BlockWithOutputs> BlockChain::GetOutputsByHeight(const uint64_t startHeight, const uint64_t maxHeight) const
{
	auto pChainStateReader = m_pChainState->Read();
	const uint64_t highestHeight = (std::min)(pChainStateReader->GetHeight(EChainType::CONFIRMED), maxHeight);

	std::vector<BlockWithOutputs> blocksWithOutputs;
	blocksWithOutputs.reserve(highestHeight - startHeight + 1);

	uint64_t height = startHeight;
	while (height <= highestHeight)
	{
		std::unique_ptr<BlockWithOutputs> pBlockWithOutputs = pChainStateReader->GetBlockWithOutputs(height);
		if (pBlockWithOutputs != nullptr)
		{
			blocksWithOutputs.push_back(*pBlockWithOutputs);
		}

		++height;
	}

	return blocksWithOutputs;
}

bool BlockChain::HasBlock(const uint64_t height, const Hash& hash) const
{
	auto pChainStateReader = m_pChainState->Read();
	
	auto pIndex = pChainStateReader->GetChainStore()->GetConfirmedChain()->GetByHeight(height);

	return pIndex != nullptr && pIndex->GetHash() == hash;
}

std::vector<std::pair<uint64_t, Hash>> BlockChain::GetBlocksNeeded(const uint64_t maxNumBlocks) const
{
	return m_pChainState->Read()->GetBlocksNeeded(maxNumBlocks);
}

bool BlockChain::ProcessNextOrphanBlock()
{
	BlockHeaderPtr pNextHeader = nullptr;
	std::shared_ptr<const FullBlock> pOrphanBlock = nullptr;

	{
		auto pReader = m_pChainState->Read();
		const uint64_t height = pReader->GetHeight(EChainType::CONFIRMED) + 1;
		pNextHeader = pReader->GetBlockHeaderByHeight(height, EChainType::CANDIDATE);
		if (pNextHeader == nullptr)
		{
			return false;
		}

		pOrphanBlock = pReader->GetOrphanBlock(height, pNextHeader->GetHash());
		if (pOrphanBlock == nullptr)
		{
			return false;
		}
	}

	try
	{
		return BlockProcessor(m_pChainState).ProcessBlock(*pOrphanBlock) == EBlockChainStatus::SUCCESS;
	}
	catch (std::exception&)
	{
		m_pChainState->
			Write()->
			GetOrphanPool()->
			RemoveOrphan(pNextHeader->GetHeight(), pNextHeader->GetHash());
		return false;
	}
}

bool BlockChain::HasOrphan(const Hash& blockHash) const
{
	auto pReader = m_pChainState->Read();
	auto pHeader = pReader->GetBlockHeaderByHash(blockHash);
	if (pHeader == nullptr) {
		return false;
	}

	return pReader->GetOrphanBlock(pHeader->GetHeight(), blockHash) != nullptr;
}

namespace BlockChainAPI
{
	BLOCK_CHAIN_API std::shared_ptr<IBlockChain> OpenBlockChain(
		const Config& config,
		std::shared_ptr<Locked<IBlockDB>> pDatabase,
		std::shared_ptr<Locked<TxHashSetManager>> pTxHashSetManager,
		std::shared_ptr<ITransactionPool> pTransactionPool,
		std::shared_ptr<Locked<IHeaderMMR>> pHeaderMMR)
	{
		// TODO: Catch exceptions thrown. If any exceptions, delete the NODE directory and resync.
		// It would also be a good idea to re-validate the MMRs on load.
		return BlockChain::Create(config, pDatabase, pTxHashSetManager, pTransactionPool, pHeaderMMR);
	}
}