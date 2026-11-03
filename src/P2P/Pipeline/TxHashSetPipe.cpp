#include "TxHashSetPipe.h"
#include <P2P/TxHashSetArchive.h>
#include "../ArchivePublication.h"
#include <P2P/Pipeline/TxHashSetLimits.h>
#include "../Connection.h"
#include "../ConnectionManager.h"
#include "../Messages/TxHashSetArchiveMessage.h"

#include <Common/Util/HexUtil.h>
#include <Common/Util/FileUtil.h>
#include <Common/Util/ThreadUtil.h>
#include <Common/Logger.h>
#include <Core/File/FileRemover.h>
#include <Core/Global.h>
#include <BlockChain/BlockChain.h>

#include <filesystem.h>
#include <chrono>
#include <fstream>

static const int BUFFER_SIZE = 128 * 1024;

TxHashSetPipe::~TxHashSetPipe()
{
	ThreadUtil::Join(m_txHashSetThread);
	ThreadUtil::JoinAll(m_sendThreads);
}

std::shared_ptr<TxHashSetPipe> TxHashSetPipe::Create(
	const ConnectionManagerPtr& pConnectionManager,
	const IBlockChain::Ptr& pBlockChain,
	SyncStatusPtr pSyncStatus)
{
	return std::shared_ptr<TxHashSetPipe>(new TxHashSetPipe(
		pConnectionManager,
		pBlockChain,
		pSyncStatus
	));
}

void TxHashSetPipe::ReceiveTxHashSet(const Connection::Ptr& pConnection, const TxHashSetArchiveMessage& archive_msg)
{
	const std::string peer = pConnection->GetPeer()->GetIPAddress().Format();
	const std::string blockHashHex = HexUtil::ConvertToHex(archive_msg.GetBlockHash().GetData());
	const bool currentRequest =
		m_pSyncStatus->IsCurrentTxHashSetRequest(peer, blockHashHex);
	const bool previouslyRequested =
		m_pSyncStatus->WasTxHashSetRequested(peer, blockHashHex);

	if (m_pSyncStatus->GetStatus() != ESyncStatus::SYNCING_TXHASHSET)
	{
		LOG_WARNING_F(
			"Ignoring {} TxHashSet from {} (status is not syncing); no ban.",
			previouslyRequested ? "late" : "unrequested",
			*pConnection);
		pConnection->DisableReceives(true);
		return;
	}

	if (!currentRequest)
	{
		// Late answer to an earlier request, or an unsolicited archive. Neither is
		// proof of malice: reject without writing/processing and without a ban.
		LOG_WARNING_F(
			"Ignoring {} TxHashSet from {} (current request does not match); no ban.",
			previouslyRequested ? "late" : "unrequested",
			*pConnection);
		pConnection->DisableReceives(true);
		return;
	}

	const bool processing = m_processing.exchange(true);
	if (processing) {
		LOG_WARNING_F(
			"Ignoring duplicate TxHashSet from {} because another archive is already being processed; no ban.",
			*pConnection);
		pConnection->DisableReceives(true);
		return;
	}

	LOG_INFO_F("Disabling receives for {}", pConnection);
	pConnection->DisableReceives(true);
	m_pSyncStatus->SetTxHashSetDownloading(true);

	ThreadUtil::Join(m_txHashSetThread);
	const uint64_t attemptId = ++m_attemptCounter;

	m_txHashSetThread = std::thread(
		Thread_ProcessTxHashSet,
		std::ref(*this),
		pConnection,
		archive_msg.GetZippedSize(),
		archive_msg.GetBlockHash(),
		attemptId
	);
}

void TxHashSetPipe::Thread_ProcessTxHashSet(
	TxHashSetPipe& pipeline,
	Connection::Ptr pConnection,
	const uint64_t zipped_size,
	const Hash blockHash,
	const uint64_t attemptId)
{
	const std::string fileName = StringUtil::Format(
		"txhashset_{}_attempt_{}.zip",
		HASH::ShortHash(blockHash),
		attemptId
	);
	const fs::path txHashSetPath = fs::temp_directory_path() / fileName;
	const auto clearDownloading = [&pipeline]() {
		pipeline.m_pSyncStatus->SetTxHashSetDownloading(false);
	};

	if (!TxHashSetLimits::IsPlausibleSize(zipped_size)) {
		LOG_ERROR_F("Rejecting TxHashSet with implausible size {}.", zipped_size);
		pipeline.m_processing = false;
		pipeline.m_pSyncStatus->UpdateStatus(ESyncStatus::TXHASHSET_SYNC_FAILED);
		clearDownloading();
		pConnection->BanPeer(EBanReason::BadTxHashSet);
		return;
	}

	try
	{
		LoggerAPI::SetThreadName("TXHASHSET_PIPE");
		LOG_TRACE("BEGIN");

		LOG_INFO_F("Downloading TxHashSet from {}", *pConnection);

		pipeline.m_pSyncStatus->UpdateDownloaded(0);
		pipeline.m_pSyncStatus->UpdateDownloadSize(zipped_size);

		SocketPtr pSocket = pConnection->GetSocket();
		pSocket->SetReceiveTimeout(10 * 1000);
		pSocket->SetDefaultOptions();
		pSocket->SetBlocking(true);
		pSocket->SetReceiveBufferSize(BUFFER_SIZE);

		std::ofstream fout;
		fout.open(txHashSetPath, std::ios::binary | std::ios::out | std::ios::trunc);

		size_t bytesReceived = 0;
		std::vector<uint8_t> buffer(BUFFER_SIZE, 0);
		auto lastProgress = std::chrono::steady_clock::now();
		while (bytesReceived < zipped_size) {
			// Monotonic no-progress watchdog for a partially received archive.
			// The socket receive timeout is only 10s; this catches a peer that
			// keeps the connection open but stops making progress.
			if (std::chrono::steady_clock::now() - lastProgress > std::chrono::minutes(10)) {
				LOG_ERROR_F(
					"TxHashSet download made no progress for 10 minutes at {}/{} bytes; "
					"aborting, no ban.",
					bytesReceived, zipped_size);
				fout.close();
				FileUtil::RemoveFile(txHashSetPath);
				pipeline.m_processing = false;
				pipeline.m_pSyncStatus->UpdateStatus(ESyncStatus::TXHASHSET_SYNC_FAILED);
				clearDownloading();
				return;
			}

			const size_t bytesToRead = (size_t)(std::min<uint64_t>(
				zipped_size - bytesReceived,
				(uint64_t)BUFFER_SIZE));

			const bool received = pConnection->ReceiveSync(buffer, bytesToRead);
			if (!received || !Global::IsRunning()) {
				LOG_INFO_F("bytesReceived: {} zipped_size: {}", bytesReceived, zipped_size);
				LOG_ERROR("Transmission ended abruptly; not treating network/truncation as peer evidence (no ban).");
				fout.close();
				FileUtil::RemoveFile(txHashSetPath);
				pipeline.m_processing = false;
				pipeline.m_pSyncStatus->UpdateStatus(ESyncStatus::TXHASHSET_SYNC_FAILED);
				clearDownloading();

				return;
			}

			fout.write((char*)&buffer[0], bytesToRead);
			bytesReceived += bytesToRead;
			lastProgress = std::chrono::steady_clock::now();

			pipeline.m_pSyncStatus->UpdateDownloaded(bytesReceived);
		}

		fout.close();

		LOG_INFO("Downloading successful");


		SyncStatusPtr pSyncStatus = pipeline.m_pSyncStatus;

		pSyncStatus->UpdateProcessingStatus(0);
		pSyncStatus->UpdateStatus(ESyncStatus::PROCESSING_TXHASHSET);

		EBlockChainStatus processStatus = pipeline.m_pBlockChain->ProcessTransactionHashSet(blockHash, txHashSetPath, *pSyncStatus);
		if (processStatus == EBlockChainStatus::INVALID)
		{
			// Local rewind/extraction failures and peer-provided invalid data are
			// currently conflated at this boundary. Do not ban until the failure is
			// classified; the archive is rejected either way.
			LOG_ERROR(
				"TxHashSet rejected as invalid; peer validity not proven, no ban "
				"(classification required before any ban).");
			pSyncStatus->UpdateStatus(ESyncStatus::TXHASHSET_SYNC_FAILED);
		}
		else
		{
			pSyncStatus->UpdateStatus(ESyncStatus::SYNCING_BLOCKS);
		}

		LOG_TRACE("END");
	}
	catch (std::exception& e)
	{
		LOG_ERROR_F(
			"Exception thrown while downloading/processing TxHashSet from {} (size {}): {}; "
			"local/ambiguous failure, no ban.",
			*pConnection, zipped_size, e.what());
		pipeline.m_pSyncStatus->UpdateStatus(ESyncStatus::TXHASHSET_SYNC_FAILED);
		clearDownloading();
		FileUtil::RemoveFile(txHashSetPath);
	}
	catch (...)
	{
		LOG_ERROR_F(
			"Unknown exception while downloading/processing TxHashSet from {}; "
			"local/ambiguous failure, no ban.",
			*pConnection);
		pipeline.m_pSyncStatus->UpdateStatus(ESyncStatus::TXHASHSET_SYNC_FAILED);
		clearDownloading();
		FileUtil::RemoveFile(txHashSetPath);
	}

	clearDownloading();
	pipeline.m_processing = false;
}

void TxHashSetPipe::SendTxHashSet(const std::shared_ptr<Connection>& pConnection, const Hash& block_hash)
{
	const time_t maxTxHashSetRequest = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now() - std::chrono::hours(2));
	if (pConnection->GetPeer()->GetLastTxHashSetRequest() > maxTxHashSetRequest) {
		LOG_WARNING_F("Peer '{}' requested multiple TxHashSet's within 2 hours.", pConnection->GetIPAddress());
		pConnection->BanPeer(EBanReason::Abusive);
		return;
	}

	LOG_INFO_F("Sending TxHashSet snapshot to {}", pConnection);
	pConnection->GetPeer()->UpdateLastTxHashSetRequest();
	pConnection->DisableSends(true);

	std::thread send_thread = std::thread(
		Thread_SendTxHashSet,
		m_pBlockChain,
		pConnection,
		block_hash
	);
	m_sendThreads.push_back(std::move(send_thread));
}

void TxHashSetPipe::Thread_SendTxHashSet(
	IBlockChain::Ptr pBlockChain,
	std::shared_ptr<Connection> pConnection,
	Hash block_hash)
{
	auto pHeader = pBlockChain->GetBlockHeaderByHash(block_hash);
	if (pHeader == nullptr) {
		return;
	}

	fs::path zipFilePath;
	bool temporaryArchive = false;

	const fs::path archivedPath = TxHashSetArchive::Path(
		Global::GetConfig().GetTxHashSetPath(),
		pHeader);
	if (ArchivePublication::IsReadyArchive(archivedPath)) {
		// A state activated at this archive horizon was saved for serving; no
		// historical rewind is needed (G-52).
		zipFilePath = archivedPath;
		LOG_INFO_F(
			"Serving archived TxHashSet for height {} ({})",
			pHeader->GetHeight(),
			zipFilePath.string());
	} else {
		try {
			zipFilePath = pBlockChain->SnapshotTxHashSet(pHeader);
			temporaryArchive = true;
		}
		catch (std::exception& e) {
			LOG_ERROR_F(
				"Snapshot TxHashSet for height {} failed: {}",
				pHeader->GetHeight(),
				e.what());
			return;
		}
		catch (...) {
			LOG_ERROR_F("Snapshot TxHashSet for height {} failed: unknown error", pHeader->GetHeight());
			return;
		}

		LOG_INFO_F(
			"Snapshot TxHashSet for height {} is {} bytes ({})",
			pHeader->GetHeight(),
			FileUtil::GetFileSize(zipFilePath),
			zipFilePath.string());
	}

	std::unique_ptr<FileRemover> pRemover;
	if (temporaryArchive) {
		pRemover = std::make_unique<FileRemover>(zipFilePath);
	}

	std::ifstream file(zipFilePath, std::ios::in | std::ios::ate | std::ios::binary);
	if (!file.is_open()) {
		if (temporaryArchive) {
			FileUtil::RemoveFile(zipFilePath);
		}
		return;
	}

	try {
		const uint64_t fileSize = FileUtil::GetFileSize(zipFilePath);
		file.seekg(0);

		pConnection->SendSync(TxHashSetArchiveMessage{ pHeader->GetHash(), pHeader->GetHeight(), fileSize });

		std::vector<uint8_t> buffer(BUFFER_SIZE, 0);

		// Note: std::istream::read sets failbit when it extracts fewer than the
		// requested number of bytes, so `while (file.read(...))` silently drops the
		// final partial chunk of every archive whose size is not a multiple of
		// BUFFER_SIZE. Loop on gcount() instead.
		while (file)
		{
			file.read((char*)buffer.data(), BUFFER_SIZE);
			const std::streamsize bytesRead = file.gcount();
			if (bytesRead <= 0) {
				break;
			}

			std::vector<uint8_t> bytesToSend(
				buffer.cbegin(),
				buffer.cbegin() + bytesRead
			);
			bool sent = pConnection->GetSocket()->SendSync(bytesToSend, false);
			if (!sent || !Global::IsRunning()) {
				throw std::runtime_error("Transmission ended abruptly");
			}
		}

		pConnection->DisableSends(false);
	}
	catch (std::exception& e) {
		LOG_ERROR_F("Exception thrown while sending TxHashSet: {}", e.what());
	}

	file.close();
	if (temporaryArchive) {
		FileUtil::RemoveFile(zipFilePath);
	}
}