#pragma once

#include <BlockChain/BlockChain.h>
#include <Core/Enums/Environment.h>
#include <Core/Global.h>
#include <P2P/P2PServer.h>
#include <P2P/Common.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <json/json.h>
#include <string>

// Grin v5.5.2 owner API: get_status.
// Mirrors api::types::Status: chain, protocol_version, user_agent, connections,
// tip {height, last_block_pushed, prev_block_to_last, total_difficulty},
// sync_status and optional sync_info. sync_status uses the grin names, not the
// legacy Grin++ ones exposed by GET /v1/status.
class GetStatusOwnerHandler : public RPCMethod
{
public:
	GetStatusOwnerHandler(const IBlockChain::Ptr& pBlockChain, const IP2PServerPtr& pP2PServer)
		: m_pBlockChain(pBlockChain), m_pP2PServer(pP2PServer) { }
	~GetStatusOwnerHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		const BlockHeaderPtr pTip = m_pBlockChain->GetTipBlockHeader(EChainType::CONFIRMED);
		if (pTip == nullptr) {
			return request.BuildError("INTERNAL_ERROR", "Failed to find tip.");
		}

		const SyncStatusConstPtr pSyncStatus = m_pP2PServer->GetSyncStatus();
		const std::pair<size_t, size_t> numConnections = m_pP2PServer->GetNumberOfConnectedPeers();

		Json::Value tip;
		tip["height"] = Json::UInt64(pTip->GetHeight());
		tip["last_block_pushed"] = pTip->GetHash().ToHex();
		tip["prev_block_to_last"] = pTip->GetPreviousHash().ToHex();
		tip["total_difficulty"] = Json::UInt64(pTip->GetTotalDifficulty());

		const ESyncStatus syncStatus = pSyncStatus->GetStatus();

		Json::Value status;
		status["chain"] = ChainShortName(Global::GetEnv());
		status["protocol_version"] = Json::UInt(P2P::PROTOCOL_VERSION);
		status["user_agent"] = P2P::USER_AGENT;
		status["connections"] = Json::UInt(static_cast<unsigned int>(numConnections.first + numConnections.second));
		status["tip"] = tip;
		status["sync_status"] = SyncStatusName(syncStatus);

		if (syncStatus == ESyncStatus::SYNCING_HEADERS) {
			Json::Value syncInfo;
			syncInfo["current_height"] = Json::UInt64(pSyncStatus->GetBlockHeight());
			syncInfo["highest_height"] = Json::UInt64(pSyncStatus->GetNetworkHeight());
			status["sync_info"] = syncInfo;
		}

		Json::Value result;
		result["Ok"] = status;
		return request.BuildResult(result);
	}

	bool ContainsSecrets() const noexcept final { return false; }

private:
	static std::string ChainShortName(const Environment environment)
	{
		switch (environment) {
			case Environment::MAINNET: return "main";
			case Environment::FLOONET: return "floonet";
			case Environment::AUTOMATED_TESTING: return "test";
		}

		return "unknown";
	}

	static std::string SyncStatusName(const ESyncStatus status)
	{
		switch (status) {
			case ESyncStatus::SYNCING_HEADERS: return "header_sync";
			case ESyncStatus::SYNCING_TXHASHSET: return "txhashset_download";
			case ESyncStatus::PROCESSING_TXHASHSET: return "txhashset_rangeproofs_validation";
			case ESyncStatus::TXHASHSET_SYNC_FAILED: return "txhashset_sync_failed";
			case ESyncStatus::SYNCING_BLOCKS: return "body_sync";
			case ESyncStatus::WAITING_FOR_PEERS: return "awaiting_peers";
			case ESyncStatus::NOT_SYNCING: return "no_sync";
		}

		return "no_sync";
	}

	IBlockChain::Ptr m_pBlockChain;
	IP2PServerPtr m_pP2PServer;
};
