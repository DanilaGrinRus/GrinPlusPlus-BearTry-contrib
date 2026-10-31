#pragma once

#include <BlockChain/BlockChain.h>
#include <Core/Models/Features.h>
#include <Core/Models/FullBlock.h>
#include <Core/Models/TransactionOutput.h>
#include <Core/Serialization/Serializer.h>
#include <Crypto/Hasher.h>
#include <Database/BlockDb.h>
#include <Database/Database.h>
#include <PMMR/TxHashSetManager.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <json/json.h>
#include <optional>
#include <string>
#include <vector>

// Grin v5.5.2 foreign API: get_outputs.
// Params: [[commits], start_height?, end_height?, include_proof?, include_merkle_proof?]
// and returns a flat array of OutputPrintable entries (not an OutputListing).
// Outputs still present in the output PMMR are unspent; spent outputs are pruned
// from the PMMR, so their body is read back from the block DB and spent=true.
// A height range is capped at MaxBlockRange blocks per request to bound the work.
class GetOutputsHandler : public RPCMethod
{
public:
	GetOutputsHandler(
		const IBlockChain::Ptr& pBlockChain,
		const TxHashSetManager::Ptr& pTxHashSetManager,
		const IDatabasePtr& pDatabase)
		: m_pBlockChain(pBlockChain),
		m_pTxHashSetManager(pTxHashSetManager),
		m_pDatabase(pDatabase) { }
	~GetOutputsHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		if (!request.GetParams().has_value()) {
			return request.BuildError(RPC::Errors::PARAMS_MISSING);
		}

		const Json::Value& params = request.GetParams().value();
		if (!params.isArray()) {
			return request.BuildError("INVALID_PARAMS", "Expected an array of parameters");
		}

		bool includeProof = false;
		if (params.size() > 3 && !params[3].isNull()) {
			includeProof = params[3].asBool();
		}

		const auto pTxHashSet = m_pTxHashSetManager->GetTxHashSet();
		const auto pBlockDB = m_pDatabase->GetBlockDB()->Read();

		Json::Value outputs(Json::arrayValue);

		// By commitment.
		if (params.size() > 0 && params[0].isArray()) {
			for (const Json::Value& commitJSON : params[0]) {
				if (!commitJSON.isString()) {
					return request.BuildError("INVALID_PARAMS", "Expected commitment hex strings");
				}

				std::unique_ptr<Commitment> pCommitment;
				try {
					pCommitment = std::make_unique<Commitment>(JsonUtil::ConvertToCommitment(commitJSON));
				} catch (const std::exception&) {
					return request.BuildError("INVALID_PARAMS", "Invalid commitment hex string");
				}
				const Commitment& commitment = *pCommitment;

				std::unique_ptr<OutputLocation> pLocation = pBlockDB->GetOutputPosition(commitment);
				if (pLocation == nullptr) {
					continue; // Ignored like the reference client.
				}

				Json::Value outputJSON;
				if (BuildOutput(commitment, *pLocation, pTxHashSet, includeProof, outputJSON)) {
					outputs.append(outputJSON);
				}
			}
		}

		// By block height range.
		const bool hasRange = params.size() > 1 && !params[1].isNull();
		if (hasRange) {
			uint64_t startHeight = JsonUtil::ConvertToUInt64(params[1]);
			uint64_t endHeight = (params.size() > 2 && !params[2].isNull())
				? JsonUtil::ConvertToUInt64(params[2])
				: startHeight;
			if (endHeight < startHeight) {
				endHeight = startHeight;
			}
			if (endHeight - startHeight >= MaxBlockRange) {
				endHeight = startHeight + MaxBlockRange - 1;
			}

			for (uint64_t height = startHeight; height <= endHeight; height++) {
				std::unique_ptr<FullBlock> pBlock = m_pBlockChain->GetBlockByHeight(height);
				if (pBlock == nullptr) {
					continue;
				}

				for (const TransactionOutput& output : pBlock->GetOutputs()) {
					std::unique_ptr<OutputLocation> pLocation = pBlockDB->GetOutputPosition(output.GetCommitment());
					if (pLocation == nullptr) {
						continue;
					}

					Json::Value outputJSON;
					if (BuildOutput(output.GetCommitment(), *pLocation, pTxHashSet, includeProof, outputJSON)) {
						outputs.append(outputJSON);
					}
				}
			}
		}

		Json::Value result;
		result["Ok"] = outputs;
		return request.BuildResult(result);
	}

	bool ContainsSecrets() const noexcept final { return false; }

private:
	static constexpr uint64_t MaxBlockRange = 100;

	bool BuildOutput(
		const Commitment& commitment,
		const OutputLocation& location,
		const std::shared_ptr<ITxHashSet>& pTxHashSet,
		const bool includeProof,
		Json::Value& outputJSON) const
	{
		EOutputFeatures features = EOutputFeatures::DEFAULT;
		std::optional<RangeProof> rangeProof;

		bool spent = true;
		if (pTxHashSet != nullptr) {
			try {
				const OutputDTO output = pTxHashSet->GetOutput(location);
				features = output.GetFeatures();
				rangeProof = output.GetRangeProof();
				spent = false;
			} catch (const std::exception&) {
				spent = true;
			}
		}

		if (spent) {
			// The output was pruned from the PMMR; recover its body from the block DB.
			std::unique_ptr<FullBlock> pBlock = m_pBlockChain->GetBlockByHeight(location.GetBlockHeight());
			if (pBlock == nullptr) {
				return false;
			}

			bool found = false;
			for (const TransactionOutput& output : pBlock->GetOutputs()) {
				if (output.GetCommitment() == commitment) {
					features = output.GetFeatures();
					rangeProof = output.GetRangeProof();
					found = true;
					break;
				}
			}

			if (!found) {
				return false;
			}
		}

		if (!rangeProof.has_value()) {
			return false;
		}

		Serializer proofSerializer;
		rangeProof->Serialize(proofSerializer);

		outputJSON["output_type"] = (features == EOutputFeatures::COINBASE_OUTPUT) ? "Coinbase" : "Transaction";
		outputJSON["commit"] = commitment.ToHex();
		outputJSON["spent"] = spent;
		if (includeProof) {
			outputJSON["proof"] = rangeProof->Format();
		} else {
			outputJSON["proof"] = Json::nullValue;
		}
		outputJSON["proof_hash"] = Hasher::Blake2b(proofSerializer.GetBytes()).ToHex();
		outputJSON["block_height"] = Json::UInt64(location.GetBlockHeight());
		outputJSON["merkle_proof"] = Json::nullValue;
		outputJSON["mmr_index"] = Json::UInt64(location.GetLeafIndex().GetPosition() + 1);
		return true;
	}

	IBlockChain::Ptr m_pBlockChain;
	TxHashSetManager::Ptr m_pTxHashSetManager;
	IDatabasePtr m_pDatabase;
};
