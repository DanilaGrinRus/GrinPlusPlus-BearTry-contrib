#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>

// Static PIBD/desegmentation parameters.
// Values are copied 1:1 from grin v5.5.2 chain/src/pibd_params.rs so that a
// C++ node segments and schedules exactly like the Rust reference. These are
// protocol constants, not user configuration.
namespace PibdParams
{
	// Segment heights assumed for requests and segment calculation.
	static constexpr uint8_t BITMAP_SEGMENT_HEIGHT = 9;
	static constexpr uint8_t OUTPUT_SEGMENT_HEIGHT = 11;
	static constexpr uint8_t RANGEPROOF_SEGMENT_HEIGHT = 11;
	static constexpr uint8_t KERNEL_SEGMENT_HEIGHT = 11;

	// Maximum number of received segments cached (across all trees) before we
	// stop requesting more.
	static constexpr size_t MAX_CACHED_SEGMENTS = 15;

	// Number of simultaneous segment requests.
	static constexpr size_t SEGMENT_REQUEST_COUNT = 15;

	// How long to wait for a requested segment before re-requesting it.
	static constexpr std::chrono::seconds SEGMENT_REQUEST_TIMEOUT{ 20 };

	// If no max-work PIBD peer is seen for this long, fall back to the legacy
	// txhashset.zip download.
	static constexpr std::chrono::seconds TXHASHSET_ZIP_FALLBACK_TIME{ 660 };
}
