#pragma once

#include <cstdint>
#include <cstring>
#include <filesystem.h>
#include <fstream>
#include <string>

// Deterministic commit point for the three append-only chain stores
// (header_mmr.bin, candidate.chain, confirmed.chain). They are still written
// separately, but after all writers have committed we atomically replace this
// small checkpoint file. On load every store is truncated to the checkpoint
// heights, so a crash between two store commits can only lose uncommitted
// headers, never leave a mismatched state that reads past the MMR.
struct DbCheckpointData
{
	uint64_t Sequence = 0;
	uint64_t MmrLeaves = 0;
	uint64_t CandidateEntries = 0;
	uint64_t ConfirmedEntries = 0;
};

namespace DbCheckpoint
{
	static constexpr uint64_t MAGIC = 0x4B4350504E495247ULL; // "GRINPPCK"
	static constexpr uint32_t VERSION = 1;

	inline uint64_t Checksum(const uint8_t* data, const size_t size) noexcept
	{
		uint64_t hash = 1469598103934665603ULL; // FNV-1a 64
		for (size_t i = 0; i < size; i++) {
			hash ^= data[i];
			hash *= 1099511628211ULL;
		}

		return hash;
	}

	inline bool Read(const fs::path& path, DbCheckpointData& out)
	{
		std::ifstream file(path, std::ios::binary);
		if (!file.is_open()) {
			return false;
		}

		uint8_t bytes[52];
		file.read(reinterpret_cast<char*>(bytes), sizeof(bytes));
		if (file.gcount() != (std::streamsize)sizeof(bytes)) {
			return false;
		}

		const uint64_t storedChecksum =
			*(reinterpret_cast<const uint64_t*>(bytes + 44));
		if (storedChecksum != Checksum(bytes, 44)) {
			return false;
		}

		const uint64_t magic = *(reinterpret_cast<const uint64_t*>(bytes));
		const uint32_t version = *(reinterpret_cast<const uint32_t*>(bytes + 8));
		if (magic != MAGIC || version != VERSION) {
			return false;
		}

		out.Sequence = *(reinterpret_cast<const uint64_t*>(bytes + 12));
		out.MmrLeaves = *(reinterpret_cast<const uint64_t*>(bytes + 20));
		out.CandidateEntries = *(reinterpret_cast<const uint64_t*>(bytes + 28));
		out.ConfirmedEntries = *(reinterpret_cast<const uint64_t*>(bytes + 36));

		// Basic sanity: a chain always has at least genesis, confirmed can not be
		// ahead of candidate.
		if (out.MmrLeaves == 0 || out.CandidateEntries == 0 ||
			out.ConfirmedEntries == 0 ||
			out.ConfirmedEntries > out.CandidateEntries) {
			return false;
		}

		return true;
	}

	inline bool Write(const fs::path& path, const DbCheckpointData& data)
	{
		uint8_t bytes[52]{ 0 };

		const uint64_t magic = MAGIC;
		const uint32_t version = VERSION;
		std::memcpy(bytes, &magic, sizeof(magic));
		std::memcpy(bytes + 8, &version, sizeof(version));
		std::memcpy(bytes + 12, &data.Sequence, sizeof(data.Sequence));
		std::memcpy(bytes + 20, &data.MmrLeaves, sizeof(data.MmrLeaves));
		std::memcpy(bytes + 28, &data.CandidateEntries, sizeof(data.CandidateEntries));
		std::memcpy(bytes + 36, &data.ConfirmedEntries, sizeof(data.ConfirmedEntries));
		const uint64_t checksum = Checksum(bytes, 44);
		std::memcpy(bytes + 44, &checksum, sizeof(checksum));

		const fs::path temp = path.string() + ".tmp";
		{
			std::ofstream file(temp, std::ios::binary | std::ios::trunc);
			if (!file.is_open()) {
				return false;
			}

			file.write(reinterpret_cast<const char*>(bytes), sizeof(bytes));
			file.flush();
			if (!file.good()) {
				return false;
			}
		}

		std::error_code ec;
		fs::rename(temp, path, ec);
		if (ec) {
			// Windows does not always replace an existing target with rename().
			fs::remove(path, ec);
			ec.clear();
			fs::rename(temp, path, ec);
			if (ec) {
				fs::remove(temp, ec);
				return false;
			}
		}

		return true;
	}

	inline std::string Describe(const DbCheckpointData& data)
	{
		return "sequence " + std::to_string(data.Sequence) +
			", mmr_leaves " + std::to_string(data.MmrLeaves) +
			", candidate " + std::to_string(data.CandidateEntries) +
			", confirmed " + std::to_string(data.ConfirmedEntries);
	}
}
