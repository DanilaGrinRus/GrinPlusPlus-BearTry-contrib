#include "RangeProofBatchVerifier.h"

#include "OutputPMMR.h"
#include "RangeProofPMMR.h"
#include "TxHashSetImpl.h"

#include <Common/Logger.h>
#include <Crypto/Bulletproofs.h>
#include <Crypto/Crypto.h>
#include <P2P/SyncStatus.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <deque>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <thread>

namespace
{
	using Batch = RangeProofBatchVerifier::Batch;
	using Stats = RangeProofBatchVerifier::Stats;

	constexpr size_t BATCH_SIZE = 1000;
	constexpr int MAX_EXPERIMENTAL_WORKERS = 4;

	long long G49NowMs()
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now().time_since_epoch()).count();
	}

	std::string ReadEnvironment(const char* name)
	{
#ifdef _WIN32
		char* buffer = nullptr;
		size_t size = 0;
		if (_dupenv_s(&buffer, &size, name) == 0 && buffer != nullptr) {
			std::string value(buffer);
			free(buffer);
			return value;
		}
		return std::string();
#else
		const char* value = std::getenv(name);
		return value == nullptr ? std::string() : std::string(value);
#endif
	}

	struct Enumeration
	{
		uint64_t Proofs = 0;
		uint64_t Batches = 0;
		uint64_t LastBatch = 0;
		uint64_t LastLeafPosition = 0;
	};

	// One sequential reader. The staging PMMRs are immutable for the duration of
	// validation (nothing writes them before activation), so the reader never
	// races a writer. `onBatch` takes ownership of each batch and returns false to
	// stop; a missing rangeproof or an exception is a hard failure.
	bool EnumerateBatches(
		TxHashSet& txHashSet,
		const uint64_t proofLimit,
		const std::function<bool(Batch&&)>& onBatch,
		Enumeration& enumeration)
	{
		Batch current;
		const uint64_t outputMmrSize = txHashSet.GetOutputPMMR()->GetSize();
		for (LeafIndex leafIdx = LeafIndex::At(0); leafIdx.GetPosition() < outputMmrSize; leafIdx++)
		{
			std::unique_ptr<OutputIdentifier> pOutput = txHashSet.GetOutputPMMR()->GetAt(leafIdx);
			if (pOutput == nullptr) {
				continue;
			}

			std::unique_ptr<RangeProof> pRangeProof = txHashSet.GetRangeProofPMMR()->GetAt(leafIdx);
			if (pRangeProof == nullptr) {
				LOG_ERROR_F("No rangeproof found at leaf index ({})", leafIdx);
				return false;
			}

			if (current.Proofs.empty()) {
				current.FirstLeafPosition = leafIdx.GetPosition();
			}
			current.LastLeafPosition = leafIdx.GetPosition();
			current.Proofs.emplace_back(pOutput->GetCommitment(), *pRangeProof);
			++enumeration.Proofs;
			enumeration.LastLeafPosition = leafIdx.GetPosition();

			if (current.Proofs.size() >= BATCH_SIZE)
			{
				enumeration.LastBatch = current.Proofs.size();
				++enumeration.Batches;
				if (!onBatch(std::move(current))) {
					return false;
				}
				current = Batch();
			}

			if (proofLimit != 0 && enumeration.Proofs >= proofLimit) {
				break;
			}
		}

		if (!current.Proofs.empty())
		{
			enumeration.LastBatch = current.Proofs.size();
			++enumeration.Batches;
			if (!onBatch(std::move(current))) {
				return false;
			}
		}

		return true;
	}

	void UpdateProgress(
		SyncStatus& syncStatus,
		const uint64_t verifiedProofs,
		const uint64_t outputMmrSize)
	{
		// Conservative progress: verified unspent proofs divided by the output MMR
		// size (an upper bound). It can never claim more than actually verified.
		double fraction = 0.0;
		if (outputMmrSize != 0) {
			fraction = static_cast<double>(verifiedProofs) / static_cast<double>(outputMmrSize);
		}
		double progress = 40.0 + (30.0 * fraction);
		if (progress > 70.0) {
			progress = 70.0;
		}
		syncStatus.UpdateProcessingStatus(static_cast<uint8_t>(progress));
	}

	long long MedianMs(std::vector<long long> values)
	{
		if (values.empty()) {
			return 0;
		}
		std::sort(values.begin(), values.end());
		return values[values.size() / 2];
	}

	void LogBatchSummary(
		const char* mode,
		const int workers,
		const Enumeration& enumeration,
		const uint64_t verifiedBatches,
		const uint64_t verifiedProofs,
		const uint64_t maxQueueDepth,
		const uint64_t queueCapacity,
		const uint64_t workerBusyMs,
		const std::vector<long long>& batchDurations,
		const long long elapsedMs)
	{
		long long minBatchMs = 0;
		long long maxBatchMs = 0;
		long long totalBatchMs = 0;
		for (const long long duration : batchDurations) {
			if (minBatchMs == 0 || duration < minBatchMs) {
				minBatchMs = duration;
			}
			if (duration > maxBatchMs) {
				maxBatchMs = duration;
			}
			totalBatchMs += duration;
		}
		const long long medianBatchMs = MedianMs(batchDurations);
		const double proofsPerSecond = elapsedMs > 0
			? (static_cast<double>(verifiedProofs) * 1000.0) / static_cast<double>(elapsedMs)
			: 0.0;

		LOG_INFO_F(
			"G49T phase=range_proofs mode={} workers={} batches={} verified_batches={} "
			"proofs={} verified_proofs={} last_batch={} last_leaf_position={} ms={} "
			"proofs_per_s={:.1f} batch_ms_min={} batch_ms_median={} batch_ms_avg={:.1f} batch_ms_max={} "
			"worker_busy_ms={} max_queue_depth={} queue_capacity={}",
			mode,
			workers,
			enumeration.Batches,
			verifiedBatches,
			enumeration.Proofs,
			verifiedProofs,
			enumeration.LastBatch,
			enumeration.LastLeafPosition,
			elapsedMs,
			proofsPerSecond,
			minBatchMs,
			medianBatchMs,
			verifiedBatches == 0 ? 0.0 : static_cast<double>(totalBatchMs) / static_cast<double>(verifiedBatches),
			maxBatchMs,
			workerBusyMs,
			maxQueueDepth,
			queueCapacity);
	}
}

int RangeProofBatchVerifier::ConfiguredWorkerCount()
{
	const std::string value = ReadEnvironment("GRINPP_RANGEPROOF_WORKERS");
	if (value.empty()) {
		return 1;
	}

	int workers = std::atoi(value.c_str());
	if (workers < 1) {
		workers = 1;
	}
	if (workers > MAX_EXPERIMENTAL_WORKERS) {
		LOG_WARNING_F("GRINPP_RANGEPROOF_WORKERS={} clamped to {}", workers, MAX_EXPERIMENTAL_WORKERS);
		workers = MAX_EXPERIMENTAL_WORKERS;
	}
	return workers;
}

uint64_t RangeProofBatchVerifier::ConfiguredProofLimit()
{
	const std::string value = ReadEnvironment("GRINPP_RANGEPROOF_LIMIT");
	if (value.empty()) {
		return 0;
	}
	return std::strtoull(value.c_str(), nullptr, 10);
}

bool RangeProofBatchVerifier::Verify(TxHashSet& txHashSet, SyncStatus& syncStatus)
{
	const int workers = ConfiguredWorkerCount();
	if (workers <= 1) {
		return VerifySequential(txHashSet, syncStatus);
	}
	return VerifyParallel(txHashSet, syncStatus, workers);
}

bool RangeProofBatchVerifier::VerifySequential(TxHashSet& txHashSet, SyncStatus& syncStatus)
{
	const long long start = G49NowMs();
	const uint64_t proofLimit = ConfiguredProofLimit();
	const uint64_t outputMmrSize = txHashSet.GetOutputPMMR()->GetSize();
	Enumeration enumeration;
	std::vector<long long> batchDurations;
	uint64_t verifiedBatches = 0;
	uint64_t verifiedProofs = 0;

	const auto onBatch = [&](Batch&& batch) -> bool {
		const long long batchStart = G49NowMs();
		if (!Crypto::VerifyRangeProofs(batch.Proofs)) {
			LOG_ERROR("Failed to verify rangeproofs");
			return false;
		}
		batchDurations.push_back(G49NowMs() - batchStart);
		++verifiedBatches;
		verifiedProofs += static_cast<uint64_t>(batch.Proofs.size());
		UpdateProgress(syncStatus, batch.LastLeafPosition, outputMmrSize);
		return true;
	};

	bool success = false;
	try {
		success = EnumerateBatches(txHashSet, proofLimit, onBatch, enumeration);
	}
	catch (const std::exception& e) {
		LOG_ERROR_F("Rangeproof reader threw: {}", e.what());
		success = false;
	}
	catch (...) {
		LOG_ERROR("Rangeproof reader threw an unknown exception");
		success = false;
	}

	long long totalBatchMs = 0;
	for (const long long duration : batchDurations) {
		totalBatchMs += duration;
	}
	LogBatchSummary(
		"sequential",
		1,
		enumeration,
		verifiedBatches,
		verifiedProofs,
		0,
		0,
		static_cast<uint64_t>(totalBatchMs),
		batchDurations,
		G49NowMs() - start);

	if (!success) {
		LOG_ERROR("G49R result=FAIL mode=sequential");
	}
	return success;
}

bool RangeProofBatchVerifier::VerifyParallel(
	TxHashSet& txHashSet,
	SyncStatus& syncStatus,
	const int workers)
{
	const long long start = G49NowMs();
	const uint64_t proofLimit = ConfiguredProofLimit();
	const uint64_t outputMmrSize = txHashSet.GetOutputPMMR()->GetSize();
	const size_t queueCapacity = std::max<size_t>(2, static_cast<size_t>(workers) * 2);

	// Independent verifier per worker: own secp256k1 context, own generators, own
	// scratch (created inside each Verify call), own cache. Upstream secp256k1.h
	// explicitly permits sharing a context between threads for concurrent
	// read-only use, but independent instances make the read-only requirement
	// structural instead of assumption-based.
	std::vector<std::unique_ptr<Bulletproofs>> verifiers;
	verifiers.reserve(static_cast<size_t>(workers));
	try {
		for (int i = 0; i < workers; ++i) {
			verifiers.push_back(std::make_unique<Bulletproofs>());
		}
	}
	catch (const std::exception& e) {
		LOG_ERROR_F("G49R failed to create worker verifiers: {}", e.what());
		return false;
	}

	Enumeration enumeration;
	Stats stats;
	const auto verifyFn = [&verifiers](const size_t workerIndex, const Batch& batch) -> bool {
		return verifiers[workerIndex]->VerifyBulletproofs(batch.Proofs);
	};
	const auto produceFn = [&](const SubmitFn& submit) -> bool {
		return EnumerateBatches(
			txHashSet,
			proofLimit,
			[&submit](Batch&& batch) -> bool { return submit(std::move(batch)); },
			enumeration);
	};
	const auto progressFn = [&syncStatus, outputMmrSize](const uint64_t verifiedProofs) {
		UpdateProgress(syncStatus, verifiedProofs, outputMmrSize);
	};

	bool success = false;
	try {
		success = Execute(workers, queueCapacity, CancelFn(), verifyFn, produceFn, progressFn, &stats);
	}
	catch (const std::exception& e) {
		LOG_ERROR_F("G49R parallel runner threw: {}", e.what());
		success = false;
	}

	if (!success && stats.ThreadStartFailed && stats.SubmittedBatches == 0) {
		// Thread creation failed before any batch was submitted. The fail-closed
		// path is a full sequential re-read; no partial state can be activated.
		LOG_WARNING("G49R parallel worker start failed; falling back to full sequential verification");
		return VerifySequential(txHashSet, syncStatus);
	}

	// Fail closed on any inconsistency between read, submitted and verified work.
	if (success) {
		success =
			stats.SubmittedBatches == enumeration.Batches &&
			stats.VerifiedBatches == stats.SubmittedBatches &&
			stats.SubmittedProofs == enumeration.Proofs &&
			stats.VerifiedProofs == enumeration.Proofs;
		if (!success) {
			LOG_ERROR_F(
				"G49R accounting mismatch: read_batches={} submitted={} verified={} read_proofs={} submitted_proofs={} verified_proofs={}",
				enumeration.Batches,
				stats.SubmittedBatches,
				stats.VerifiedBatches,
				enumeration.Proofs,
				stats.SubmittedProofs,
				stats.VerifiedProofs);
		}
	}

	LogBatchSummary(
		"parallel",
		workers,
		enumeration,
		stats.VerifiedBatches,
		stats.VerifiedProofs,
		stats.MaxQueueDepth,
		queueCapacity,
		stats.WorkerBusyMs,
		stats.BatchDurations,
		G49NowMs() - start);

	if (!success) {
		LOG_ERROR_F("G49R result=FAIL mode=parallel workers={}", workers);
	}
	return success;
}

bool RangeProofBatchVerifier::Execute(
	const int workers,
	size_t queueCapacity,
	const CancelFn& cancelled,
	const BatchFn& verify,
	const ProduceFn& produce,
	const ProgressFn& progress,
	Stats* pStats)
{
	Stats stats;
	if (workers < 2) {
		LOG_ERROR_F("G49R Execute requires at least 2 workers, got {}", workers);
		if (pStats != nullptr) {
			*pStats = stats;
		}
		return false;
	}
	if (queueCapacity == 0) {
		queueCapacity = 1;
	}

	std::mutex queueMutex;
	std::condition_variable notEmpty;
	std::condition_variable notFull;
	std::deque<Batch> queue;
	bool done = false;
	bool failed = false;
	std::string failureReason;

	std::mutex durationsMutex;
	std::vector<long long> batchDurations;

	std::mutex progressMutex;
	uint64_t lastReportedProgress = 0;

	std::atomic<uint64_t> submittedBatches{ 0 };
	std::atomic<uint64_t> submittedProofs{ 0 };
	std::atomic<uint64_t> verifiedBatches{ 0 };
	std::atomic<uint64_t> verifiedProofs{ 0 };
	std::atomic<uint64_t> maxQueueDepth{ 0 };
	std::atomic<uint64_t> workerBusyMs{ 0 };

	const auto failNow = [&](const std::string& reason) {
		{
			std::lock_guard<std::mutex> lock(queueMutex);
			if (!failed) {
				failed = true;
				failureReason = reason;
			}
		}
		notEmpty.notify_all();
		notFull.notify_all();
	};

	const auto workerLoop = [&](const size_t workerIndex) {
		LOG_INFO_F("G49R worker={} started", workerIndex);
		while (true)
		{
			Batch batch;
			{
				std::unique_lock<std::mutex> lock(queueMutex);
				notEmpty.wait(lock, [&]() { return failed || !queue.empty() || done; });
				if (failed) {
					return;
				}
				if (queue.empty()) {
					if (done) {
						return;
					}
					continue;
				}
				batch = std::move(queue.front());
				queue.pop_front();
				notFull.notify_one();
			}

			if (cancelled && cancelled()) {
				failNow("cancelled before batch verification");
				return;
			}

			const long long batchStart = G49NowMs();
			bool verified = false;
			try {
				verified = verify(workerIndex, batch);
			}
			catch (const std::exception& e) {
				LOG_ERROR_F("G49R worker={} threw: {}", workerIndex, e.what());
				verified = false;
			}
			catch (...) {
				LOG_ERROR_F("G49R worker={} threw an unknown exception", workerIndex);
				verified = false;
			}
			const long long duration = G49NowMs() - batchStart;
			{
				std::lock_guard<std::mutex> lock(durationsMutex);
				batchDurations.push_back(duration);
			}
			workerBusyMs.fetch_add(static_cast<uint64_t>(duration));

			if (!verified) {
				failNow("batch verification failed");
				return;
			}

			verifiedBatches.fetch_add(1);
			const uint64_t proofCount = static_cast<uint64_t>(batch.Proofs.size());
			const uint64_t totalVerified = verifiedProofs.fetch_add(proofCount) + proofCount;
			if (progress) {
				std::lock_guard<std::mutex> lock(progressMutex);
				if (totalVerified > lastReportedProgress) {
					lastReportedProgress = totalVerified;
					progress(totalVerified);
				}
			}
		}
	};

	std::vector<std::thread> threads;
	threads.reserve(static_cast<size_t>(workers));
	try {
		for (int i = 0; i < workers; ++i) {
			threads.emplace_back(workerLoop, static_cast<size_t>(i));
		}
	}
	catch (const std::exception& e) {
		LOG_ERROR_F("G49R failed to start workers: {}", e.what());
		{
			std::lock_guard<std::mutex> lock(queueMutex);
			failed = true;
			failureReason = "worker thread creation failed";
		}
		notEmpty.notify_all();
		notFull.notify_all();
		for (std::thread& thread : threads) {
			if (thread.joinable()) {
				thread.join();
			}
		}
		stats.ThreadStartFailed = true;
		if (pStats != nullptr) {
			*pStats = stats;
		}
		return false;
	}

	bool produced = false;
	try {
		produced = produce([&](Batch&& batch) -> bool {
			if (batch.Proofs.empty()) {
				failNow("empty batch submitted");
				return false;
			}

			const size_t proofCount = batch.Proofs.size();
			std::unique_lock<std::mutex> lock(queueMutex);
			notFull.wait(lock, [&]() { return failed || queue.size() < queueCapacity; });
			if (failed) {
				return false;
			}
			if (cancelled && cancelled()) {
				lock.unlock();
				failNow("cancelled before batch submission");
				return false;
			}

			queue.emplace_back(std::move(batch));
			submittedBatches.fetch_add(1);
			submittedProofs.fetch_add(static_cast<uint64_t>(proofCount));
			const uint64_t depth = static_cast<uint64_t>(queue.size());
			uint64_t previousDepth = maxQueueDepth.load();
			while (depth > previousDepth &&
				!maxQueueDepth.compare_exchange_weak(previousDepth, depth)) {
			}
			notEmpty.notify_one();
			return true;
		});
	}
	catch (const std::exception& e) {
		LOG_ERROR_F("G49R producer threw: {}", e.what());
		failNow("producer threw");
		produced = false;
	}
	catch (...) {
		failNow("producer threw an unknown exception");
		produced = false;
	}

	{
		std::lock_guard<std::mutex> lock(queueMutex);
		done = true;
	}
	notEmpty.notify_all();
	notFull.notify_all();

	for (std::thread& thread : threads) {
		if (thread.joinable()) {
			thread.join();
		}
	}

	bool success = produced && !failed;
	if (success && cancelled && cancelled()) {
		success = false;
		LOG_ERROR("G49R cancelled after the producer finished");
	}
	if (success) {
		success =
			verifiedBatches.load() == submittedBatches.load() &&
			verifiedProofs.load() == submittedProofs.load();
		if (!success) {
			LOG_ERROR_F(
				"G49R Execute accounting mismatch: submitted_batches={} verified_batches={} submitted_proofs={} verified_proofs={}",
				submittedBatches.load(),
				verifiedBatches.load(),
				submittedProofs.load(),
				verifiedProofs.load());
		}
	}

	if (!success && !failed) {
		failNow("incomplete submission set");
	}

	stats.SubmittedBatches = submittedBatches.load();
	stats.VerifiedBatches = verifiedBatches.load();
	stats.SubmittedProofs = submittedProofs.load();
	stats.VerifiedProofs = verifiedProofs.load();
	stats.MaxQueueDepth = maxQueueDepth.load();
	stats.WorkerBusyMs = workerBusyMs.load();
	{
		std::lock_guard<std::mutex> lock(durationsMutex);
		stats.BatchDurations = batchDurations;
	}

	if (pStats != nullptr) {
		*pStats = stats;
	}

	LOG_INFO_F(
		"G49R execute workers={} queue_capacity={} submitted_batches={} verified_batches={} "
		"submitted_proofs={} verified_proofs={} max_queue_depth={} worker_busy_ms={} failed={} reason={}",
		workers,
		queueCapacity,
		stats.SubmittedBatches,
		stats.VerifiedBatches,
		stats.SubmittedProofs,
		stats.VerifiedProofs,
		stats.MaxQueueDepth,
		stats.WorkerBusyMs,
		failed ? "yes" : "no",
		failed ? failureReason : "");

	return success;
}

bool RangeProofBatchVerifier::SelfTest(std::string* pError)
{
	const auto setError = [pError](const std::string& message) {
		if (pError != nullptr) {
			*pError = message;
		}
	};

	const auto runCase = [&](const std::string& name, const bool expected, const bool actual,
		const std::string& detail) {
		const bool pass = (expected == actual);
		if (pass) {
			LOG_INFO_F("G49SELFTEST case={} result=PASS {}", name, detail);
		}
		else {
			LOG_ERROR_F("G49SELFTEST case={} result=FAIL expected={} actual={} {}",
				name, expected ? "true" : "false", actual ? "true" : "false", detail);
			setError(name + ": " + detail);
		}
		return pass;
	};

	const auto runBatches = [](const size_t count, const size_t lastCount,
		const RangeProofBatchVerifier::BatchFn& verify) {
		return [count, lastCount, verify](const RangeProofBatchVerifier::SubmitFn& submit) {
			for (size_t i = 0; i < count; ++i) {
				const size_t proofs = (i + 1 == count) ? lastCount : 3;
				RangeProofBatchVerifier::Batch batch;
				batch.FirstLeafPosition = i;
				batch.LastLeafPosition = i;
				for (size_t j = 0; j < proofs; ++j) {
					Commitment commitment;
					std::vector<unsigned char> proofBytes(8, static_cast<unsigned char>(i));
					batch.Proofs.emplace_back(commitment, RangeProof(std::move(proofBytes)));
				}
				if (!submit(std::move(batch))) {
					return false;
				}
			}
			return true;
		};
	};

	bool allPassed = true;

	// 1. Success path with an intentionally partial last batch.
	{
		RangeProofBatchVerifier::Stats stats;
		std::vector<unsigned char> seen;
		std::mutex seenMutex;
		const RangeProofBatchVerifier::BatchFn verify =
			[&](const size_t, const RangeProofBatchVerifier::Batch& batch) {
				std::lock_guard<std::mutex> lock(seenMutex);
				seen.push_back(batch.Proofs.front().second.GetProofBytes().front());
				return true;
			};
		const bool ok = Execute(2, 2, CancelFn(), verify, runBatches(7, 2, verify), ProgressFn(), &stats);
		allPassed = runCase("success_last_partial", true, ok,
			"submitted=" + std::to_string(stats.SubmittedBatches) +
			" verified=" + std::to_string(stats.VerifiedBatches) +
			" proofs=" + std::to_string(stats.VerifiedProofs)) && allPassed;
		std::vector<unsigned char> sortedSeen = seen;
		std::sort(sortedSeen.begin(), sortedSeen.end());
		const bool noGapsOrDuplicates =
			sortedSeen.size() == 7 &&
			std::adjacent_find(sortedSeen.begin(), sortedSeen.end()) == sortedSeen.end() &&
			sortedSeen.front() == 0 && sortedSeen.back() == 6;
		allPassed = runCase("success_no_gaps_or_duplicates", true, noGapsOrDuplicates,
			"seen=" + std::to_string(seen.size())) && allPassed;
		allPassed = runCase("success_counts_match", true,
			stats.SubmittedBatches == 7 && stats.VerifiedBatches == 7 &&
			stats.SubmittedProofs == stats.VerifiedProofs &&
			stats.VerifiedProofs == static_cast<uint64_t>(20),
			"submitted_proofs=" + std::to_string(stats.SubmittedProofs)) && allPassed;
	}

	// 2. A failing batch must fail the whole run.
	{
		RangeProofBatchVerifier::Stats stats;
		const RangeProofBatchVerifier::BatchFn verify =
			[&](const size_t, const RangeProofBatchVerifier::Batch& batch) {
				return batch.Proofs.front().second.GetProofBytes().front() != 3;
			};
		const bool ok = Execute(2, 2, CancelFn(), verify, runBatches(8, 3, verify), ProgressFn(), &stats);
		allPassed = runCase("batch_failure_fails_overall", true, ok == false,
			"verified_batches=" + std::to_string(stats.VerifiedBatches)) && allPassed;
	}

	// 3. A worker exception must fail the whole run.
	{
		RangeProofBatchVerifier::Stats stats;
		const RangeProofBatchVerifier::BatchFn verify =
			[&](const size_t, const RangeProofBatchVerifier::Batch& batch) -> bool {
				if (batch.Proofs.front().second.GetProofBytes().front() == 2) {
					throw std::runtime_error("injected worker exception");
				}
				return true;
			};
		const bool ok = Execute(2, 2, CancelFn(), verify, runBatches(8, 3, verify), ProgressFn(), &stats);
		allPassed = runCase("worker_exception_fails_overall", true, ok == false,
			"verified_batches=" + std::to_string(stats.VerifiedBatches)) && allPassed;
	}

	// 4. Cancellation is an overall failure.
	{
		RangeProofBatchVerifier::Stats stats;
		std::atomic<bool> cancel{ false };
		const RangeProofBatchVerifier::BatchFn verify =
			[&](const size_t, const RangeProofBatchVerifier::Batch&) {
				cancel = true;
				return true;
			};
		const RangeProofBatchVerifier::CancelFn cancelled = [&cancel]() { return cancel.load(); };
		const bool ok = Execute(2, 2, cancelled, verify, runBatches(8, 3, verify), ProgressFn(), &stats);
		allPassed = runCase("cancellation_fails_overall", true, ok == false,
			"submitted=" + std::to_string(stats.SubmittedBatches)) && allPassed;
	}

	// 5. Empty batch is a hard failure, never silently skipped.
	{
		RangeProofBatchVerifier::Stats stats;
		const RangeProofBatchVerifier::BatchFn verify =
			[](const size_t, const RangeProofBatchVerifier::Batch&) { return true; };
		const RangeProofBatchVerifier::ProduceFn produce =
			[](const RangeProofBatchVerifier::SubmitFn& submit) {
				RangeProofBatchVerifier::Batch empty;
				return submit(std::move(empty));
			};
		const bool ok = Execute(2, 2, CancelFn(), verify, produce, ProgressFn(), &stats);
		allPassed = runCase("empty_batch_fails_overall", true, ok == false,
			"submitted=" + std::to_string(stats.SubmittedBatches)) && allPassed;
	}

	// 6. The queue depth must stay bounded under a slow worker.
	{
		RangeProofBatchVerifier::Stats stats;
		const RangeProofBatchVerifier::BatchFn verify =
			[](const size_t, const RangeProofBatchVerifier::Batch&) {
				std::this_thread::sleep_for(std::chrono::milliseconds(2));
				return true;
			};
		const bool ok = Execute(2, 2, CancelFn(), verify, runBatches(40, 1, verify), ProgressFn(), &stats);
		allPassed = runCase("bounded_queue", true, ok && stats.MaxQueueDepth <= 2,
			"max_queue_depth=" + std::to_string(stats.MaxQueueDepth)) && allPassed;
	}

	// 7. Fewer than two workers is rejected by the runner.
	{
		RangeProofBatchVerifier::Stats stats;
		const bool ok = Execute(1, 2, CancelFn(),
			[](const size_t, const RangeProofBatchVerifier::Batch&) { return true; },
			[](const RangeProofBatchVerifier::SubmitFn&) { return true; },
			ProgressFn(), &stats);
		allPassed = runCase("workers_below_two_rejected", true, ok == false, "") && allPassed;
	}

	return allPassed;
}
