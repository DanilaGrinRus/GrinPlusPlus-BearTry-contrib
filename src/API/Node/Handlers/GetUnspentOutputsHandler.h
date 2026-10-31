#pragma once

#include <BlockChain/BlockChain.h>
#include <Core/Models/DTOs/OutputDTO.h>
#include <Database/Database.h>
#include <PMMR/TxHashSetManager.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <json/json.h>
#include <algorithm>
#include <optional>

// Grin v5.5.2 foreign API: get_unspent_outputs.
// Params: [start_index, end_index?, max, include_proof?].
// The output PMMR drops spent leaves (TxHashSet::ApplyBlock removes them), so the
// outputs present in the MMR are exactly the unspent ones and spent is always false.
// max is capped at 10_000 like the reference handler.
class GetUnspentOutputsHandler : public RPCMethod
{
public:
	GetUnspentOutputsHandler(const TxHashSetManager::Ptr& pTxHashSetManager, const IDatabasePtr& pDatabase)
		: m_pTxHashSetManager(pTxHashSetManager), m_pDatabase(pDatabase) { }
	~GetUnspentOutputsHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		if (!request.GetParams().has_value()) {
			return request.BuildError(RPC::Errors::PARAMS_MISSING);
		}

		const Json::Value& params = request.GetParams().value();
		if (!params.isArray() || params.empty() || params[0].isNull()) {
			return request.BuildError("INVALID_PARAMS", "Expected at least 1 parameter: start_index");
		}

		const uint64_t startIndex = JsonUtil::ConvertToUInt64(params[0]);

		std::optional<uint64_t> endIndex;
		if (params.size() > 1 && !params[1].isNull()) {
			endIndex = JsonUtil::ConvertToUInt64(params[1]);
		}

		uint64_t max = 1000;
		if (params.size() > 2 && !params[2].isNull()) {
			max = JsonUtil::ConvertToUInt64(params[2]);
		}
		max = (std::min)(max == 0 ? 1000 : max, (uint64_t)10000);

		bool includeProof = false;
		if (params.size() > 3 && !params[3].isNull()) {
			includeProof = params[3].asBool();
		}

		const auto pTxHashSet = m_pTxHashSetManager->GetTxHashSet();
		if (pTxHashSet == nullptr) {
			return request.BuildError("INTERNAL_ERROR", "TxHashSet is not available.");
		}

		const BlockHeaderPtr pFlushed = pTxHashSet->GetFlushedBlockHeader();
		if (pFlushed == nullptr) {
			return request.BuildError("INTERNAL_ERROR", "TxHashSet has no flushed header.");
		}
		const uint64_t highestIndex = pFlushed->GetOutputMMRSize();

		uint64_t lastIndex = endIndex.value_or(highestIndex);
		lastIndex = (std::min)(lastIndex, highestIndex);

		Json::Value outputs(Json::arrayValue);
		uint64_t lastRetrieved = startIndex;

		const auto pBlockDB = m_pDatabase->GetBlockDB()->Read();
		const uint64_t chunkSize = 512;
		uint64_t cursor = startIndex;
		while (cursor <= lastIndex && outputs.size() < max) {
			const uint64_t chunkEnd = (std::min)(lastIndex, cursor + chunkSize - 1);
			for (const OutputDTO& output : pTxHashSet->GetOutputsByMMRIndex(pBlockDB.GetShared(), cursor, chunkEnd)) {
				if (outputs.size() >= max) {
					break;
				}

				Json::Value outputJSON = output.ToJSON();
				outputJSON["spent"] = false;
				if (!includeProof) {
					outputJSON["proof"] = Json::nullValue;
				}

				outputs.append(outputJSON);
				lastRetrieved = output.GetLocation().GetLeafIndex().GetPosition() + 1;
			}

			cursor = chunkEnd + 1;
		}

		Json::Value listing;
		listing["last_retrieved_index"] = Json::UInt64(lastRetrieved);
		listing["highest_index"] = Json::UInt64(highestIndex);
		listing["outputs"] = outputs;

		Json::Value result;
		result["Ok"] = listing;
		return request.BuildResult(result);
	}

	bool ContainsSecrets() const noexcept final { return false; }

private:
	TxHashSetManager::Ptr m_pTxHashSetManager;
	IDatabasePtr m_pDatabase;
};
