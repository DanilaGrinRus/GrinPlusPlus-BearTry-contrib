#include "TxHashSetProcessor.h"

#include <Common/Logger.h>
#include <Database/BlockDb.h>
#include <Core/Models/BlockHeader.h>
#include <BlockChain/BlockChain.h>
#include <Common/Util/StringUtil.h>
#include <Common/Util/HexUtil.h>
#include <PMMR/TxHashSetStaging.h>
#include <fstream>

TxHashSetProcessor::TxHashSetProcessor(
	const Config& config,
	IBlockChain& blockChain,
	std::shared_ptr<Locked<ChainState>> pChainState)
	: m_config(config),
	m_blockChain(blockChain),
	m_pChainState(pChainState)
{

}

bool TxHashSetProcessor::ProcessTxHashSet(const Hash& blockHash, const fs::path& path, SyncStatus& syncStatus)
{
	auto pHeader = m_pChainState->Read()->GetBlockHeaderByHash(blockHash);
	if (pHeader == nullptr)
	{
		LOG_ERROR_F("Header not found for hash {}.", blockHash);
		return false;
	}

	const fs::path livePath = m_config.GetTxHashSetPath();
	TxHashSetStaging::Recover(livePath);
	const fs::path stagingPath = TxHashSetStaging::Staging(livePath);
	const fs::path backupPath = TxHashSetStaging::Backup(livePath);

	std::error_code ec;
	fs::remove_all(stagingPath, ec);

	const auto reopenLive = [&]() {
		const auto pTip = m_pChainState->Read()->GetChainStore()->GetConfirmedChain()->GetTip();
		if (pTip == nullptr) {
			return;
		}

		const auto pTipHeader = m_pChainState->Read()->GetBlockDB()->GetBlockHeader(pTip->GetHash());
		if (pTipHeader != nullptr) {
			m_pChainState->Write()->GetTxHashSetManager()->Open(pTipHeader);
		}
	};

	// 1. Extract and validate into a staging directory; the live state is untouched.
	m_pChainState->Write()->GetTxHashSetManager()->Close();
	ITxHashSetPtr pStaged = TxHashSetManager::LoadFromZip(m_config, path, pHeader, stagingPath);
	if (pStaged == nullptr)
	{
		LOG_ERROR_F("Failed to load {}", path);
		fs::remove_all(stagingPath, ec);
		reopenLive();
		return false;
	}

	// 2. Validate the whole staged state.
	auto pBlockSums = pStaged->ValidateTxHashSet(*pHeader, m_blockChain, syncStatus);
	if (pBlockSums == nullptr)
	{
		LOG_ERROR_F("Validation of {} failed; keeping the existing state.", path);
		pStaged.reset();
		fs::remove_all(stagingPath, ec);
		reopenLive();
		return false;
	}

	// 3. Release file handles before renaming the directories.
	pStaged.reset();

	// 4. Atomic swap with a marker for crash recovery.
	fs::remove_all(backupPath, ec);
	{
		std::ofstream marker(TxHashSetStaging::Marker(livePath), std::ios::binary | std::ios::trunc);
		marker << "pending";
	}

	try {
		if (fs::exists(livePath, ec)) {
			fs::rename(livePath, backupPath);
		}
		fs::rename(stagingPath, livePath);
		fs::remove_all(backupPath, ec);
		fs::remove(TxHashSetStaging::Marker(livePath), ec);
	}
	catch (std::exception& e) {
		LOG_ERROR_F("Failed to activate the staged txhashset: {}", e.what());
		ec.clear();
		if (!fs::exists(livePath, ec) && fs::exists(backupPath, ec)) {
			fs::rename(backupPath, livePath, ec);
		}
		ec.clear();
		fs::remove(TxHashSetStaging::Marker(livePath), ec);
		reopenLive();
		return false;
	}

	// 5. Open the installed state and publish block sums/output positions.
	auto pManager = m_pChainState->Write()->GetTxHashSetManager();
	ITxHashSetPtr pLive = pManager->Open(pHeader);
	if (pLive == nullptr) {
		LOG_ERROR_F("Failed to open the activated txhashset for {}.", path);
		return false;
	}

	auto pChainStateBatch = m_pChainState->BatchWrite();
	pChainStateBatch->GetBlockDB()->AddBlockSums(pHeader->GetHash(), *pBlockSums);
	LOG_DEBUG("Saving output positions.");
	pLive->SaveOutputPositions(
		pChainStateBatch->GetChainStore()->GetCandidateChain(),
		pChainStateBatch->GetBlockDB());

	LOG_DEBUG("Using TxHashSet.");
	pChainStateBatch->GetTxHashSetManager()->SetTxHashSet(pLive);

	LOG_DEBUG("Updating confirmed chain.");
	if (!UpdateConfirmedChain(pChainStateBatch, *pHeader))
	{
		LOG_ERROR_F("Failed to update confirmed chain for {}.", path);
		pChainStateBatch->GetTxHashSetManager()->Close();
		return false;
	}

	pChainStateBatch->Commit();
	return true;
}
bool TxHashSetProcessor::ProcessExistingTxHashSet(const Hash& blockHash, SyncStatus& syncStatus)
{
	auto pHeader = m_pChainState->Read()->GetBlockHeaderByHash(blockHash);
	if (pHeader == nullptr) {
		LOG_ERROR_F("Header not found for hash {}.", blockHash);
		return false;
	}

	// 1. Close the existing txhashset and open the activated files.
	m_pChainState->Write()->GetTxHashSetManager()->Close();
	ITxHashSetPtr pTxHashSet = m_pChainState->Write()->GetTxHashSetManager()->Open(pHeader);
	if (pTxHashSet == nullptr) {
		LOG_ERROR("Failed to open the activated txhashset.");
		return false;
	}

	// 2. Validate the entire txhashset against the horizon header.
	auto pBlockSums = pTxHashSet->ValidateTxHashSet(*pHeader, m_blockChain, syncStatus);
	if (pBlockSums == nullptr) {
		LOG_ERROR("Validation of the activated txhashset failed.");
		return false;
	}

	// 3. Store block sums and output positions, then install the txhashset.
	auto pChainStateBatch = m_pChainState->BatchWrite();
	pChainStateBatch->GetBlockDB()->AddBlockSums(pHeader->GetHash(), *pBlockSums);
	pTxHashSet->SaveOutputPositions(
		pChainStateBatch->GetChainStore()->GetCandidateChain(), pChainStateBatch->GetBlockDB());
	pChainStateBatch->GetTxHashSetManager()->SetTxHashSet(pTxHashSet);

	// 4. Update the confirmed chain to the horizon.
	if (!UpdateConfirmedChain(pChainStateBatch, *pHeader)) {
		LOG_ERROR("Failed to update the confirmed chain for the activated txhashset.");
		pChainStateBatch->GetTxHashSetManager()->Close();
		return false;
	}

	pChainStateBatch->Commit();
	return true;
}
bool TxHashSetProcessor::UpdateConfirmedChain(Writer<ChainState> pLockedState, const BlockHeader& blockHeader)
{
	std::shared_ptr<Chain> pCandidateChain = pLockedState->GetChainStore()->GetCandidateChain();
	std::shared_ptr<Chain> pConfirmedChain = pLockedState->GetChainStore()->GetConfirmedChain();
	
	std::shared_ptr<const BlockIndex> pBlockIndex = pCandidateChain->GetByHeight(blockHeader.GetHeight());
	if (pBlockIndex->GetHash() != blockHeader.GetHash())
	{
		return false;
	}

	std::shared_ptr<const BlockIndex> pCommonIndex = pLockedState->GetChainStore()->FindCommonIndex(EChainType::CANDIDATE, EChainType::CONFIRMED);
	pConfirmedChain->Rewind(pCommonIndex->GetHeight());

	uint64_t height = pCommonIndex->GetHeight() + 1;
	while (height <= pBlockIndex->GetHeight())
	{
		pConfirmedChain->AddBlock(pCandidateChain->GetHash(height), height);
		height++;
	}

	return true;
}