#include "MessageProcessor.h"
#include "BlockLocator.h"
#include "ConnectionManager.h"
#include "Pipeline/Pipeline.h"

// Network Messages
#include "Messages/ErrorMessage.h"
#include "Messages/PingMessage.h"
#include "Messages/PongMessage.h"
#include "Messages/GetPeerAddressesMessage.h"
#include "Messages/PeerAddressesMessage.h"
#include "Messages/BanReasonMessage.h"

// BlockChain Messages
#include "Messages/GetHeadersMessage.h"
#include "Messages/HeaderMessage.h"
#include "Messages/HeadersMessage.h"
#include "Messages/GetHeaderSegmentMessage.h"
#include "Messages/HeaderSegmentMessage.h"
#include "Sync/PihdClient.h"
#include "Messages/BlockMessage.h"
#include "Messages/GetBlockMessage.h"
#include "Messages/CompactBlockMessage.h"
#include "Messages/GetCompactBlockMessage.h"

// PIBD Messages
#include "Messages/SegmentRequestMessage.h"
#include "Messages/SegmentResponseMessage.h"
#include "PIBD/PibdSegmentService.h"
#include "PIBD/PibdSync.h"
#include "../PMMR/TxHashSetImpl.h"

// Transaction Messages
#include "Messages/TransactionMessage.h"
#include "Messages/StemTransactionMessage.h"
#include "Messages/TxHashSetRequestMessage.h"
#include "Messages/TxHashSetArchiveMessage.h"
#include "Messages/GetTransactionMessage.h"
#include "Messages/TransactionKernelMessage.h"

#include <Core/File/FileRemover.h>
#include <Core/Exceptions/BadDataException.h>
#include <Core/Exceptions/BlockChainException.h>
#include <Core/Global.h>
#include <P2P/Common.h>
#include <Common/Util/HexUtil.h>
#include <Common/Util/StringUtil.h>
#include <Common/Util/FileUtil.h>
#include <BlockChain/BlockChain.h>
#include <Common/Logger.h>
#include <thread>
#include <fstream>

static const int BUFFER_SIZE = 256 * 1024;

using namespace MessageTypes;

MessageProcessor::MessageProcessor(
	ConnectionManager& connectionManager,
	Locked<PeerManager> peerManager,
	const IBlockChain::Ptr& pBlockChain,
	const std::shared_ptr<Pipeline>& pPipeline,
	SyncStatusConstPtr pSyncStatus,
	std::shared_ptr<PibdSync> pPibdSync,
	std::shared_ptr<PihdClient> pPihdClient)
	: m_connectionManager(connectionManager),
	m_peerManager(peerManager),
	m_pBlockChain(pBlockChain),
	m_pPipeline(pPipeline),
	m_pSyncStatus(pSyncStatus),
	m_pPibdSync(std::move(pPibdSync)),
	m_pPihdClient(std::move(pPihdClient))
{

}

uint64_t MessageProcessor::GetBlockHeight() const
{
	return m_pBlockChain->GetHeight(EChainType::CONFIRMED);
}

uint64_t MessageProcessor::GetBlockDifficulty() const
{
	return m_pBlockChain->GetTotalDifficulty(EChainType::CONFIRMED);
}

void MessageProcessor::ProcessMessage(const std::shared_ptr<Connection>& pConnection, const RawMessage& rawMessage)
{
	const EMessageType messageType = rawMessage.GetMessageHeader().GetMessageType();

	LOG_DEBUG_F("Received {} from {}", MessageTypes::ToString(messageType), pConnection);

	try
	{
		return ProcessMessageInternal(pConnection, rawMessage);
	}
	catch (const BadDataException& e)
	{
		LOG_ERROR_F(
			"{} from {} contained bad data: {}",
			MessageTypes::ToString(messageType),
			pConnection,
			e.what()
		);

		// grin 5.5.2 has no ban-reason field on the wire; the reason is logged
		// locally before the peer is banned.
		pConnection->BanPeer(e.GetReason());
		if (m_pSyncStatus != nullptr) {
			m_pSyncStatus->IncrementBansInvalidData();
		}
	}
	catch (const BlockChainException& e)
	{
		LOG_ERROR_F(
			"BlockChain exception while processing {} from {}: {}",
			MessageTypes::ToString(messageType),
			pConnection,
			e.what()
		);
	}
	catch (const DeserializationException& e)
	{
		LOG_ERROR_F(
			"Deserialization exception while processing {} from {}: {}",
			MessageTypes::ToString(messageType),
			pConnection,
			e.what()
		);
		pConnection->Disconnect();
	}
}

void MessageProcessor::ProcessMessageInternal(const std::shared_ptr<Connection>& pConnection, const RawMessage& rawMessage)
{
	const MessageHeader& header = rawMessage.GetMessageHeader();
	EProtocolVersion protocolVersion = pConnection->GetProtocolVersion();
	ByteBuffer byteBuffer(rawMessage.GetPayload(), protocolVersion);

	switch (header.GetMessageType())
	{
		case Error:
		{
			const ErrorMessage errorMessage = ErrorMessage::Deserialize(byteBuffer);
			LOG_WARNING_F("Error message \"{}\" retrieved from {}", errorMessage.GetErrorMessage(), pConnection);
			break;
		}
		case BanReasonMsg:
		{
			const BanReasonMessage banReasonMessage = BanReasonMessage::Deserialize(byteBuffer);
			std::string banMessage = BanReason::Format((EBanReason)banReasonMessage.GetBanReason());
			LOG_WARNING_F("BanReason message '{}' retrieved from {}", banMessage, pConnection);
			break;
		}
		case Ping:
		{
			const PingMessage pingMessage = PingMessage::Deserialize(byteBuffer);
			pConnection->UpdateTotals(pingMessage.GetTotalDifficulty(), pingMessage.GetHeight());

			// Answer from the authoritative chain store. Tip-header lookup can
			// lag immediately after PIBD activation, leaving the peer with height 0.
			const uint64_t height = m_pBlockChain->GetHeight(EChainType::CONFIRMED);
			const uint64_t difficulty = m_pBlockChain->GetTotalDifficulty(EChainType::CONFIRMED);
			const PongMessage pongMessage(difficulty, height);
			pConnection->SendAsync(pongMessage);
			break;
		}
		case Pong:
		{
			const PongMessage pongMessage = PongMessage::Deserialize(byteBuffer);
			pConnection->UpdateTotals(pongMessage.GetTotalDifficulty(), pongMessage.GetHeight());
			break;
		}
		case GetHeaderSegment:
		{
			// Client-only build: we never advertise PIHD_HIST, so serving a
			// header segment is deliberately unsupported.
			LOG_DEBUG_F("Ignoring GetHeaderSegment from {}: PIHD server disabled", pConnection);
			break;
		}
		case HeaderSegment:
		{
			if (m_pPihdClient == nullptr) {
				break;
			}
			const HeaderSegmentMessage response = HeaderSegmentMessage::Deserialize(byteBuffer);
			m_pPihdClient->OnHeaderSegment(pConnection, response);
			break;
		}
		case OutputBitmapSegment:
		case OutputSegment:
		case RangeProofSegment:
		case KernelSegment:
		{
			if (m_pPibdSync == nullptr || !m_pPibdSync->IsActive()) {
				LOG_DEBUG_F(
					"Ignoring {} from {}: no PIBD sync in progress",
					MessageTypes::ToString(header.GetMessageType()),
					pConnection);
				break;
			}

			const SegmentResponseMessage segmentResponse =
				SegmentResponseMessage::Deserialize(byteBuffer, header.GetMessageType());
			if (!m_pPibdSync->OnResponse(segmentResponse, std::chrono::steady_clock::now())) {
				LOG_DEBUG_F("PIBD response from {} was not applied", pConnection);
			}

			break;
		}
		case GetOutputBitmapSegment:
		case GetOutputSegment:
		case GetRangeProofSegment:
		case GetKernelSegment:
		{
			// Serve the installed flushed snapshot. A node can still be catching
			// up on later blocks; the snapshot at H is immutable once flushed and
			// is exactly what a Rust peer asks for. Requests for another H are
			// rejected below by the flushed-hash comparison.
			try {
				const SegmentRequestMessage request =
					SegmentRequestMessage::Deserialize(byteBuffer, header.GetMessageType());
				const Hash& blockHash = request.GetBlockHash();
				const SegmentIdentifier& identifier = request.GetIdentifier();

				// Limit work per peer-controlled request. The Grin 5.5.2 PIBD
				// client uses heights 9 and 11; height 13 retains headroom.
				// Larger segment requests require a separately bounded implementation.
				if (identifier.GetHeight() > 13) {
					LOG_WARNING_F("Ignoring PIBD request with oversized segment height {}", identifier.GetHeight());
					break;
				}

				if (!pConnection->AllowPibdRequest()) {
					LOG_DEBUG_F("Throttling PIBD request from {}: per-connection limit reached", pConnection);
					break;
				}

				// Build the entire response from one API snapshot and re-check the
				// state sequence after the expensive segment/root reads. A block
				// apply/reorg in between forces a retry; the response is never
				// assembled from two different states.
				bool served = false;
				for (int attempt = 0; attempt < 3 && !served; ++attempt) {
					const IBlockChain::ApiStateSnapshot snapshot = m_pBlockChain->GetApiStateSnapshot();
					if (!snapshot.valid || snapshot.txHashSet == nullptr) {
						LOG_DEBUG_F("Cannot serve {}: no API state snapshot",
							MessageTypes::ToString(header.GetMessageType()));
						break;
					}

					const std::shared_ptr<TxHashSet> pState =
						std::dynamic_pointer_cast<TxHashSet>(snapshot.txHashSet);
					if (pState == nullptr) {
						LOG_DEBUG_F("Cannot serve {}: no txhashset is open",
							MessageTypes::ToString(header.GetMessageType()));
						break;
					}

					const BlockHeaderPtr pFlushedHeader = pState->GetFlushedBlockHeader();
					if (pFlushedHeader == nullptr || pFlushedHeader->GetHeight() == 0 ||
						pFlushedHeader->GetHash() != blockHash) {
						LOG_DEBUG_F(
							"Ignoring {} from {}: requested horizon is not our flushed state",
							MessageTypes::ToString(header.GetMessageType()),
							pConnection);
						break;
					}

					PibdSegmentService service(
						pState->GetOutputPMMR(), pState->GetRangeProofPMMR(), pState->GetKernelMMR());

					std::shared_ptr<SegmentResponseMessage> pResponse;
					switch (header.GetMessageType()) {
						case GetOutputBitmapSegment:
							pResponse = std::make_shared<SegmentResponseMessage>(
								SegmentResponseMessage::CreateOutputBitmap(
									blockHash, service.GetBitmapSegment(identifier), service.GetOutputMmrRoot()));
							break;
						case GetOutputSegment:
							pResponse = std::make_shared<SegmentResponseMessage>(
								SegmentResponseMessage::CreateOutputs(
									blockHash, service.GetOutputSegment(identifier), service.GetBitmapMmrRoot()));
							break;
						case GetRangeProofSegment:
							pResponse = std::make_shared<SegmentResponseMessage>(
								SegmentResponseMessage::CreateRangeProofs(
									blockHash, service.GetRangeProofSegment(identifier)));
							break;
						case GetKernelSegment:
							pResponse = std::make_shared<SegmentResponseMessage>(
								SegmentResponseMessage::CreateKernels(
									blockHash, service.GetKernelSegment(identifier)));
							break;
						default:
							break;
					}

					if (pResponse == nullptr) {
						break;
					}

					if (!m_pBlockChain->IsApiStateCurrent(snapshot.sequence)) {
						LOG_DEBUG_F(
							"PIBD snapshot changed while serving {} from {}; retrying",
							MessageTypes::ToString(header.GetMessageType()),
							pConnection);
						continue;
					}

					pConnection->SendAsync(*pResponse);
					served = true;
				}

				if (!served) {
					LOG_DEBUG_F("PIBD response for {} was not sent (state changed or unavailable)",
						MessageTypes::ToString(header.GetMessageType()));
				}
			}
			catch (const std::exception& e) {
				LOG_WARNING_F(
					"Failed to serve {} from {}: {}",
					MessageTypes::ToString(header.GetMessageType()),
					pConnection,
					e.what());
			}

			break;
		}		case GetPeerAddrs:
		{
			const GetPeerAddressesMessage getPeerAddressesMessage = GetPeerAddressesMessage::Deserialize(byteBuffer);
			const Capabilities capabilities = getPeerAddressesMessage.GetCapabilities();

			const std::vector<PeerPtr> peers = m_peerManager.Read()->GetPeers(capabilities.GetCapability(), P2P::MAX_PEER_ADDRS);
			std::vector<SocketAddress> socketAddresses;
			std::transform(
				peers.cbegin(),
				peers.cend(),
				std::back_inserter(socketAddresses),
				[this](const PeerPtr& peer) { return SocketAddress(peer->GetIPAddress(), Global::GetConfig().GetP2PPort()); }
			);

			LOG_TRACE_F("Sending {} addresses to {}.", socketAddresses.size(), pConnection);
			pConnection->SendAsync(PeerAddressesMessage{ std::move(socketAddresses) });
			break;
		}
		case PeerAddrs:
		{
			const PeerAddressesMessage peerAddressesMessage = PeerAddressesMessage::Deserialize(byteBuffer);
			const std::vector<SocketAddress>& peerAddresses = peerAddressesMessage.GetPeerAddresses();

			LOG_TRACE_F("Received {} addresses from {}.", peerAddresses.size(), pConnection);
			m_peerManager.Write()->AddFreshPeers(peerAddresses);
			break;
		}
		case GetHeaders:
		{
			const GetHeadersMessage getHeadersMessage = GetHeadersMessage::Deserialize(byteBuffer);
			const std::vector<Hash>& hashes = getHeadersMessage.GetHashes();

			std::vector<BlockHeaderPtr> blockHeaders = BlockLocator(m_pBlockChain).LocateHeaders(hashes);
			LOG_DEBUG_F("Sending {} headers to {}.", blockHeaders.size(), pConnection);
                
			pConnection->SendAsync(HeadersMessage{ std::move(blockHeaders) });
			break;
		}
		case Header:
		{
			auto pBlockHeader = HeaderMessage::Deserialize(byteBuffer).GetHeader();

			if (pBlockHeader->GetTotalDifficulty() > pConnection->GetTotalDifficulty()) {
				pConnection->UpdateTotals(pBlockHeader->GetTotalDifficulty(), pBlockHeader->GetHeight());
			}

			if (m_pSyncStatus->GetStatus() != ESyncStatus::NOT_SYNCING) {
				break;
			}

			const EBlockChainStatus status = m_pBlockChain->AddBlockHeader(pBlockHeader);
			if (status == EBlockChainStatus::SUCCESS || status == EBlockChainStatus::ALREADY_EXISTS || status == EBlockChainStatus::ORPHANED) {
				if (!m_pBlockChain->HasBlock(pBlockHeader->GetHeight(), pBlockHeader->GetHash())) {
					LOG_TRACE_F("Valid header {} received from {}. Requesting compact block", *pBlockHeader, pConnection);
					const GetCompactBlockMessage getCompactBlockMessage(pBlockHeader->GetHash());
					pConnection->SendAsync(GetCompactBlockMessage{ pBlockHeader->GetHash() });
				}
			} else if (status == EBlockChainStatus::INVALID) {
				pConnection->BanPeer(EBanReason::BadBlockHeader);
			} else {
				LOG_TRACE_F("Header {} from {} not needed", *pBlockHeader, pConnection);
			}

			break;
		}
		case Headers:
		{
			const HeadersMessage headersMessage = HeadersMessage::Deserialize(byteBuffer);
			const std::vector<BlockHeaderPtr>& blockHeaders = headersMessage.GetHeaders();

			LOG_DEBUG_F("{} headers received from {}", blockHeaders.size(), pConnection);

			const EBlockChainStatus status = m_pBlockChain->AddBlockHeaders(blockHeaders);
			if (status == EBlockChainStatus::INVALID) {
				pConnection->BanPeer(EBanReason::BadBlockHeader);
			}

			LOG_DEBUG_F("Headers message from {} finished processing", pConnection);
			break;
		}
		case GetBlock:
		{
			const GetBlockMessage getBlockMessage = GetBlockMessage::Deserialize(byteBuffer);
			std::unique_ptr<FullBlock> pBlock = m_pBlockChain->GetBlockByHash(getBlockMessage.GetHash());
			if (pBlock != nullptr) {
				pConnection->SendAsync(BlockMessage{ std::move(*pBlock) });
			}

			break;
		}
		case Block:
		{
			const BlockMessage blockMessage = BlockMessage::Deserialize(byteBuffer);
			const FullBlock& block = blockMessage.GetBlock();

			LOG_TRACE_F("Block received: {}", block.GetHeight());

			if (m_pSyncStatus->GetStatus() == ESyncStatus::SYNCING_BLOCKS) {
				m_pPipeline->ProcessBlock(*pConnection, block);
			} else {
				const EBlockChainStatus added = m_pBlockChain->AddBlock(block);
				if (added == EBlockChainStatus::SUCCESS) {
					const HeaderMessage headerMessage(block.GetHeader());
					m_connectionManager.BroadcastMessage(headerMessage, pConnection->GetId());
				} else if (added == EBlockChainStatus::ORPHANED) {
					if (block.GetTotalDifficulty() > m_pBlockChain->GetTotalDifficulty(EChainType::CONFIRMED))
					{
						pConnection->SendAsync(GetCompactBlockMessage{ block.GetPreviousHash() });
					}
				}
			}

			break;
		}
		case GetCompactBlock:
		{
			const GetCompactBlockMessage getCompactBlockMessage = GetCompactBlockMessage::Deserialize(byteBuffer);
			std::unique_ptr<CompactBlock> pCompactBlock = m_pBlockChain->GetCompactBlockByHash(getCompactBlockMessage.GetHash());
			if (pCompactBlock != nullptr)
			{
				pConnection->SendAsync(CompactBlockMessage{ *pCompactBlock });
			}

			break;
		}
		case CompactBlockMsg:
		{
			const CompactBlockMessage compactBlockMessage = CompactBlockMessage::Deserialize(byteBuffer);
			const CompactBlock& compactBlock = compactBlockMessage.GetCompactBlock();

			const EBlockChainStatus added = m_pBlockChain->AddCompactBlock(compactBlock);
			if (added == EBlockChainStatus::SUCCESS)
			{
				const HeaderMessage headerMessage(compactBlock.GetHeader());
				m_connectionManager.BroadcastMessage(headerMessage, pConnection->GetId());
				break;
			}
			else if (added == EBlockChainStatus::TRANSACTIONS_MISSING)
			{
				pConnection->SendAsync(GetBlockMessage{ compactBlock.GetHash() });
				break;
			}
			else if (added == EBlockChainStatus::ORPHANED)
			{
				if (compactBlock.GetHeight() < (m_pBlockChain->GetHeight(EChainType::CONFIRMED) + 100))
				{
					if (compactBlock.GetTotalDifficulty() > m_pBlockChain->GetTotalDifficulty(EChainType::CONFIRMED))
					{
						pConnection->SendAsync(GetCompactBlockMessage{ compactBlock.GetPreviousHash() });
					}
				}
			}

			break;
		}
		case StemTransaction:
		{
			if (m_pSyncStatus->GetStatus() == ESyncStatus::NOT_SYNCING) {
				TransactionPtr pTransaction = StemTransactionMessage::Deserialize(byteBuffer).GetTransaction();
				m_pPipeline->ProcessTransaction(*pConnection, pTransaction, EPoolType::STEMPOOL);
			}

			break;
		}
		case TransactionMsg:
		{
			if (m_pSyncStatus->GetStatus() == ESyncStatus::NOT_SYNCING) {
				TransactionPtr pTransaction = TransactionMessage::Deserialize(byteBuffer).GetTransaction();
				m_pPipeline->ProcessTransaction(*pConnection, pTransaction, EPoolType::MEMPOOL);
			}

			break;
		}
		case TxHashSetRequest:
		{
			TxHashSetRequestMessage msg = TxHashSetRequestMessage::Deserialize(byteBuffer);
			m_pPipeline->SendTxHashSet(pConnection, msg.GetBlockHash());
			break;
		}
		case TxHashSetArchive:
		{
			m_pPipeline->ReceiveTxHashSet(pConnection, TxHashSetArchiveMessage::Deserialize(byteBuffer));
			break;
		}
		case GetTransactionMsg:
		{
			const GetTransactionMessage getTransactionMessage = GetTransactionMessage::Deserialize(byteBuffer);
			const Hash& kernelHash = getTransactionMessage.GetKernelHash();
			LOG_DEBUG_F("Transaction with kernel {} requested.", kernelHash);

			TransactionPtr pTransaction = m_pBlockChain->GetTransactionByKernelHash(kernelHash);
			if (pTransaction != nullptr) {
				LOG_DEBUG_F("Transaction {} found.", pTransaction);
				pConnection->SendAsync(TransactionMessage{ pTransaction });
			}

			break;
		}
		case TransactionKernelMsg:
		{
			if (m_pSyncStatus->GetStatus() != ESyncStatus::NOT_SYNCING)
			{
				break;
			}

			Hash kernelHash = TransactionKernelMessage::Deserialize(byteBuffer).GetKernelHash();
			TransactionPtr pTransaction = m_pBlockChain->GetTransactionByKernelHash(kernelHash);
			if (pTransaction == nullptr) {
				pConnection->SendAsync(GetTransactionMessage{ std::move(kernelHash) });
			}

			break;
		}
		default:
		{
			break;
		}
	}

	return;
}