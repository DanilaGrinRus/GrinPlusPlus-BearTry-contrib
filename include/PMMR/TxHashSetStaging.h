#pragma once

#include <Common/Logger.h>
#include <filesystem.h>
#include <fstream>
#include <system_error>

// Atomic installation of a downloaded legacy txhashset. The zip is extracted and
// validated in a sibling staging directory; only then the live directory is
// renamed to a backup and the staging directory becomes live. A marker file makes
// a crash between the two renames recoverable on the next start (G-21).
namespace TxHashSetStaging
{
	inline fs::path Marker(const fs::path& live)
	{
		return live.parent_path() / "txhashset_activation.pending";
	}

	inline fs::path Staging(const fs::path& live)
	{
		return live.string() + ".staging";
	}

	inline fs::path Backup(const fs::path& live)
	{
		return live.string() + ".backup";
	}

	inline void Recover(const fs::path& live)
	{
		std::error_code ec;
		const fs::path marker = Marker(live);
		if (!fs::exists(marker, ec)) {
			return;
		}

		const fs::path staging = Staging(live);
		const fs::path backup = Backup(live);

		// The live directory is missing only between the two renames; restore it.
		if (!fs::exists(live, ec) && fs::exists(backup, ec)) {
			ec.clear();
			fs::rename(backup, live, ec);
			if (ec) {
				LOG_ERROR_F("Failed to restore the txhashset backup: {}", ec.message());
				return;
			}
		}

		ec.clear();
		if (fs::exists(staging, ec)) {
			ec.clear();
			fs::remove_all(staging, ec);
		}

		// The backup is only needed while the live directory is missing. Once the
		// live state exists (restored or untouched), drop a stale backup.
		ec.clear();
		if (fs::exists(live, ec)) {
			ec.clear();
			fs::remove_all(backup, ec);
		}

		ec.clear();
		fs::remove(marker, ec);
		LOG_WARNING("Recovered a partially installed txhashset after an unclean shutdown.");
	}
}
