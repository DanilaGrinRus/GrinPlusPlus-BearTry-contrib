#pragma once

#include <Core/Models/Commitment.h>
#include <Crypto/Models/RangeProof.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>

class TxHashSet;
class SyncStatus;

// G-49 experimental parallel rangeproof verification.
//
// Invariants:
//  - one sequential reader enumerates the immutable staging PMMRs;
//  - a bounded queue holds at most a fixed number of batches;
//  - each worker verifies through an independent Bulletproofs instance
//    (own secp256k1 context, generators and scratch space);
//  - any failed batch, worker exception, read error, cancellation or
//    incomplete submission set makes the whole operation fail closed;
//  - the caller only reports success after every produced batch verified and
//    the verified counts equal the read counts.
class RangeProofBatchVerifier
{
public:
	struct Batch
	{
		uint64_t FirstLeafPosition = 0;
		uint64_t LastLeafPosition = 0;
		std::vector<std::pair<Commitment, RangeProof>> Proofs;
	};

	struct Stats
	{
		uint64_t SubmittedBatches = 0;
		uint64_t VerifiedBatches = 0;
		uint64_t SubmittedProofs = 0;
		uint64_t VerifiedProofs = 0;
		uint64_t MaxQueueDepth = 0;
		uint64_t WorkerBusyMs = 0;
		bool ThreadStartFailed = false;
		std::vector<long long> BatchDurations;
	};

	using BatchFn = std::function<bool(size_t workerIndex, const Batch& batch)>;
	using SubmitFn = std::function<bool(Batch&& batch)>;
	using ProduceFn = std::function<bool(const SubmitFn& submit)>;
	using ProgressFn = std::function<void(uint64_t verifiedProofs)>;
	using CancelFn = std::function<bool()>;

	// GRINPP_RANGEPROOF_WORKERS: default 1 (sequential path). Values above 4
	// are clamped to 4; 2 or 4 are the only experimental parallel settings.
	static int ConfiguredWorkerCount();
	static uint64_t ConfiguredProofLimit();

	static bool Verify(TxHashSet& txHashSet, SyncStatus& syncStatus);
	static bool VerifySequential(TxHashSet& txHashSet, SyncStatus& syncStatus);
	static bool VerifyParallel(TxHashSet& txHashSet, SyncStatus& syncStatus, int workers);

	// Generic bounded run loop. Shared by the real verification path and by the
	// synthetic self-tests (workers, failure injection, cancellation, queue bound).
	static bool Execute(
		int workers,
		size_t queueCapacity,
		const CancelFn& cancelled,
		const BatchFn& verify,
		const ProduceFn& produce,
		const ProgressFn& progress,
		Stats* pStats);

	static bool SelfTest(std::string* pError);
};
