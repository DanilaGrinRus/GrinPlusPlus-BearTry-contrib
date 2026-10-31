#pragma once

#include <BlockChain/BlockChain.h>
#include <Database/BlockDb.h>
#include <Database/Database.h>
#include <PMMR/TxHashSetManager.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <json/json.h>
#include <algorithm>
#include <optional>

#include "PrintableSerialization.h"

// Grin v5.5.2 foreign API: get_blocks.
// Params: [start_height, end_height?, max?, include_proof?] ->
// BlockListing {last_retrieved_height, blocks: [BlockPrintable]} where
// BlockPrintable is the flat {header, inputs, outputs, kernels} shape.
// max is capped at BLOCK_TRANSFER_LIMIT (1000) like the reference handler.
class GetBlocksHandler : public RPCMethod
{
public:
	GetBlocksHandler(
		const IBlockChain::Ptr& pBlockChain,
		const TxHashSetManager::Ptr& pTxHashSetManager,
		const IDatabasePtr& pDatabase)
		: m_pBlockChain(pBlockChain),
		m_pTxHashSetManager(pTxHashSetManager),
		m_pDatabase(pDatabase) { }
	~GetBlocksHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		if (!request.GetParams().has_value()) {
			return request.BuildError(RPC::Errors::PARAMS_MISSING);
		}

		const Json::Value& params = request.GetParams().value();
		if (!params.isArray() || params.empty() || params[0].isNull()) {
			return request.BuildError("INVALID_PARAMS", "Expected at least 1 parameter: start_height");
		}

		const uint64_t startHeight = JsonUtil::ConvertToUInt64(params[0]);

		uint64_t endHeight = 0;
		if (params.size() > 1 && !params[1].isNull()) {
			endHeight = JsonUtil::ConvertToUInt64(params[1]);
		} else {
			const BlockHeaderPtr pTip = m_pBlockChain->GetTipBlockHeader(EChainType::CONFIRMED);
			if (pTip == nullptr) {
				return request.BuildError("INTERNAL_ERROR", "Failed to find tip.");
			}
			endHeight = pTip->GetHeight();
		}

		if (endHeight < startHeight) {
			return request.BuildError("INVALID_PARAMS", "end_height is before start_height");
		}

		uint64_t max = BlockTransferLimit;
		if (params.size() > 2 && !params[2].isNull()) {
			max = JsonUtil::ConvertToUInt64(params[2]);
		}
		max = (std::min)(max == 0 ? BlockTransferLimit : max, (uint64_t)BlockTransferLimit);

		bool includeProof = false;
		if (params.size() > 3 && !params[3].isNull()) {
			includeProof = params[3].asBool();
		}

		const auto pTxHashSet = m_pTxHashSetManager->GetTxHashSet();
		const auto pBlockDB = m_pDatabase->GetBlockDB()->Read();

		Json::Value blocks(Json::arrayValue);
		uint64_t lastRetrievedHeight = startHeight;

		for (uint64_t height = startHeight; height <= endHeight; height++) {
			lastRetrievedHeight = height;

			std::unique_ptr<FullBlock> pBlock = m_pBlockChain->GetBlockByHeight(height);
			if (pBlock == nullptr) {
				continue;
			}

			Json::Value blockJSON;
			blockJSON["header"] = PrintableSerialization::HeaderPrintable(*pBlock->GetHeader());

			Json::Value inputs(Json::arrayValue);
			for (const TransactionInput& input : pBlock->GetInputs()) {
				inputs.append(input.GetCommitment().ToHex());
			}
			blockJSON["inputs"] = inputs;

			Json::Value outputs(Json::arrayValue);
			for (const TransactionOutput& output : pBlock->GetOutputs()) {
				const std::unique_ptr<OutputLocation> pLocation = pBlockDB->GetOutputPosition(output.GetCommitment());
				if (pLocation == nullptr) {
					continue;
				}

				const std::optional<Json::Value> printable = PrintableSerialization::OutputPrintable(
					output.GetCommitment(), *pLocation, pTxHashSet, m_pBlockChain, includeProof);
				if (printable.has_value()) {
					outputs.append(*printable);
				}
			}
			blockJSON["outputs"] = outputs;

			Json::Value kernels(Json::arrayValue);
			for (const TransactionKernel& kernel : pBlock->GetKernels()) {
				kernels.append(PrintableSerialization::KernelPrintable(kernel));
			}
			blockJSON["kernels"] = kernels;

			blocks.append(blockJSON);
			if (blocks.size() >= max) {
				break;
			}
		}

		Json::Value listing;
		listing["last_retrieved_height"] = Json::UInt64(lastRetrievedHeight);
		listing["blocks"] = blocks;

		Json::Value result;
		result["Ok"] = listing;
		return request.BuildResult(result);
	}

	bool ContainsSecrets() const noexcept final { return false; }

private:
	static constexpr uint64_t BlockTransferLimit = 1000;

	IBlockChain::Ptr m_pBlockChain;
	TxHashSetManager::Ptr m_pTxHashSetManager;
	IDatabasePtr m_pDatabase;
};
