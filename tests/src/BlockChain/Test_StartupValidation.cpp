#include <catch.hpp>

#include <BlockChain/BlockChain.h>
#include <Core/Config.h>
#include <Core/Global.h>
#include <Database/Database.h>
#include <PMMR/Common/MmrStructure.h>
#include <PMMR/HeaderMMR.h>
#include <PMMR/TxHashSetManager.h>
#include <TxPool/TransactionPool.h>

#include <filesystem.h>
#include <fstream>
#include <iterator>
#include <map>
#include <string>
#include <vector>

namespace
{
	struct NodeComponents
	{
		IDatabasePtr Database;
		std::shared_ptr<Locked<TxHashSetManager>> TxHashSetManager;
		ITransactionPool::Ptr TxPool;
		std::shared_ptr<Locked<IHeaderMMR>> HeaderMMR;
	};

	using FileSnapshot = std::map<std::string, std::vector<uint8_t>>;

	std::vector<uint8_t> ReadFileBytes(const fs::path& path)
	{
		std::ifstream input(path, std::ios::binary);
		REQUIRE(input.is_open());

		return std::vector<uint8_t>(
			std::istreambuf_iterator<char>(input),
			std::istreambuf_iterator<char>()
		);
	}

	FileSnapshot SnapshotChainDir(const fs::path& chainPath)
	{
		FileSnapshot snapshot;
		for (const auto& entry : fs::directory_iterator(chainPath)) {
			if (!entry.is_regular_file()) {
				continue;
			}

			snapshot[entry.path().filename().string()] = ReadFileBytes(entry.path());
		}

		return snapshot;
	}

	uint64_t FileRecords(const fs::path& path)
	{
		return fs::file_size(path) / 32;
	}

	void AppendZeros(const fs::path& path, const uint64_t bytes)
	{
		std::ofstream output(path, std::ios::binary | std::ios::app);
		REQUIRE(output.is_open());

		const std::vector<char> zeros(bytes, 0);
		output.write(zeros.data(), (std::streamsize)zeros.size());
		output.close();
	}

	void ResizeRecords(const fs::path& path, const uint64_t records)
	{
		fs::resize_file(path, records * 32);
	}

	NodeComponents OpenComponents(const Config& config)
	{
		NodeComponents components;
		components.Database = DatabaseAPI::OpenDatabase(config);
		components.TxHashSetManager = std::make_shared<Locked<TxHashSetManager>>(
			std::make_shared<TxHashSetManager>(config));
		components.TxPool = TxPoolAPI::CreateTransactionPool(config);
		components.HeaderMMR = HeaderMMRAPI::OpenHeaderMMR(config);
		return components;
	}

	IBlockChain::Ptr OpenBlockChain(const Config& config, NodeComponents& components)
	{
		return BlockChainAPI::OpenBlockChain(
			config,
			components.Database->GetBlockDB(),
			components.TxHashSetManager,
			components.TxPool,
			components.HeaderMMR
		);
	}

	// Builds the smallest valid store: genesis in the chain indexes and one
	// header MMR leaf. No mining is required for the count validation.
	void CreateFreshChain()
	{
		const Config& config = Global::GetConfig();
		fs::remove_all(config.GetDataDirectory() / "NODE");

		fs::create_directories(config.GetChainPath());
		fs::create_directories(config.GetDatabasePath());
		fs::create_directories(config.GetTxHashSetPath() / "kernel");
		fs::create_directories(config.GetTxHashSetPath() / "output");
		fs::create_directories(config.GetTxHashSetPath() / "rangeproof");

		NodeComponents components = OpenComponents(config);
		const IBlockChain::Ptr pBlockChain = OpenBlockChain(config, components);
		REQUIRE(pBlockChain != nullptr);
	}

	// Reopening runs the same code path as node startup (BlockChain::Create).
	bool ReopenBlockChain(std::string& error)
	{
		try {
			const Config& config = Global::GetConfig();
			NodeComponents components = OpenComponents(config);
			const IBlockChain::Ptr pBlockChain = OpenBlockChain(config, components);
			return pBlockChain != nullptr;
		}
		catch (const std::exception& e) {
			error = e.what();
			return false;
		}
	}
}

TEST_CASE("StartupValidation::ConsistentChainReopensWithoutChangingFiles")
{
	CreateFreshChain();
	const fs::path chainPath = Global::GetConfig().GetChainPath();
	const FileSnapshot before = SnapshotChainDir(chainPath);

	std::string error;
	const bool reopened = ReopenBlockChain(error);
	INFO("reopen error: " << error);
	REQUIRE(reopened);
	REQUIRE(error.empty());

	REQUIRE(SnapshotChainDir(chainPath) == before);
}

TEST_CASE("StartupValidation::RefusesLostMmrAndPreservesFiles")
{
	CreateFreshChain();
	const fs::path chainPath = Global::GetConfig().GetChainPath();
	const fs::path mmrPath = chainPath / "header_mmr.bin";

	REQUIRE(FileRecords(mmrPath) == 1);
	// Genesis is in the chain index and the block DB, but the MMR is empty.
	// Re-initialising it would append genesis a second time; refuse instead.
	ResizeRecords(mmrPath, 0);

	const FileSnapshot corrupted = SnapshotChainDir(chainPath);
	std::string error;
	const bool reopened = ReopenBlockChain(error);

	REQUIRE_FALSE(reopened);
	REQUIRE(error.find("contains genesis but the header MMR is empty") != std::string::npos);
	REQUIRE(SnapshotChainDir(chainPath) == corrupted);
}

TEST_CASE("StartupValidation::RefusesMmrAheadAndPreservesFiles")
{
	CreateFreshChain();
	const fs::path chainPath = Global::GetConfig().GetChainPath();
	const fs::path mmrPath = chainPath / "header_mmr.bin";

	REQUIRE(FileRecords(mmrPath) == 1);
	const uint64_t targetNodes = MmrStructure::NodesForLeaves(2);
	AppendZeros(mmrPath, (targetNodes - 1) * 32);

	const FileSnapshot corrupted = SnapshotChainDir(chainPath);
	std::string error;
	const bool reopened = ReopenBlockChain(error);

	REQUIRE_FALSE(reopened);
	REQUIRE(error.find("Header MMR is ahead of the candidate chain") != std::string::npos);
	REQUIRE(SnapshotChainDir(chainPath) == corrupted);
}

TEST_CASE("StartupValidation::RefusesMmrBehindAndPreservesFiles")
{
	CreateFreshChain();
	const fs::path chainPath = Global::GetConfig().GetChainPath();
	const fs::path candidatePath = chainPath / "candidate.chain";

	REQUIRE(FileRecords(candidatePath) == 1);
	// One extra candidate-chain entry without a matching MMR leaf simulates a
	// crash after the chain-index commit. The validator must refuse before any
	// header is read; the dummy hash is never dereferenced.
	AppendZeros(candidatePath, 32);

	const FileSnapshot corrupted = SnapshotChainDir(chainPath);
	std::string error;
	const bool reopened = ReopenBlockChain(error);

	REQUIRE_FALSE(reopened);
	REQUIRE(error.find("Candidate chain is ahead of the header MMR") != std::string::npos);
	REQUIRE(SnapshotChainDir(chainPath) == corrupted);
}

TEST_CASE("StartupValidation::RefusesInvalidMmrNodeCountAndPreservesFiles")
{
	CreateFreshChain();
	const fs::path chainPath = Global::GetConfig().GetChainPath();
	const fs::path mmrPath = chainPath / "header_mmr.bin";

	// Two nodes cannot describe any MMR: 2L - popcount(L) is never 2.
	REQUIRE_FALSE(MmrStructure::IsValidNodeCount(2));
	ResizeRecords(mmrPath, 2);

	const FileSnapshot corrupted = SnapshotChainDir(chainPath);
	std::string error;
	const bool reopened = ReopenBlockChain(error);

	REQUIRE_FALSE(reopened);
	REQUIRE(error.find("not a valid MMR size") != std::string::npos);
	REQUIRE(SnapshotChainDir(chainPath) == corrupted);
}

TEST_CASE("StartupValidation::RefusesConfirmedAheadAndPreservesFiles")
{
	CreateFreshChain();
	const fs::path chainPath = Global::GetConfig().GetChainPath();
	const fs::path confirmedPath = chainPath / "confirmed.chain";

	REQUIRE(FileRecords(confirmedPath) == 1);
	ResizeRecords(confirmedPath, 2);

	const FileSnapshot corrupted = SnapshotChainDir(chainPath);
	std::string error;
	const bool reopened = ReopenBlockChain(error);

	REQUIRE_FALSE(reopened);
	REQUIRE(error.find("Confirmed chain is ahead of the candidate chain") != std::string::npos);
	REQUIRE(SnapshotChainDir(chainPath) == corrupted);
}
