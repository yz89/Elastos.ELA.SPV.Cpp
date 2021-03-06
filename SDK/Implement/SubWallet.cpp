// Copyright (c) 2012-2018 The Elastos Open Source Project
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "MasterWallet.h"
#include "SubWallet.h"

#include <Common/Utils.h>
#include <Common/Log.h>
#include <Common/ErrorChecker.h>
#include <Plugin/Transaction/TransactionOutput.h>
#include <Plugin/Transaction/TransactionInput.h>
#include <Plugin/Transaction/IDTransaction.h>
#include <Plugin/Transaction/Payload/TransferAsset.h>
#include <Account/SubAccount.h>
#include <WalletCore/Base58.h>
#include <WalletCore/CoinInfo.h>
#include <SpvService/Config.h>
#include <Wallet/UTXO.h>

#include <algorithm>
#include <boost/scoped_ptr.hpp>

namespace fs = boost::filesystem;

#define DB_FILE_EXTENSION ".db"

namespace Elastos {
	namespace ElaWallet {

		SubWallet::SubWallet(const CoinInfoPtr &info,
							 const ChainConfigPtr &config,
							 MasterWallet *parent,
							 const std::string &netType) :
				PeerManager::Listener(),
				_parent(parent),
				_info(info),
				_config(config),
				_callback(nullptr) {

			fs::path subWalletDBPath = _parent->GetDataPath();
			subWalletDBPath /= _info->GetChainID() + DB_FILE_EXTENSION;

			SubAccountPtr subAccount = SubAccountPtr(new SubAccount(_parent->_account, _config->Index()));
			_walletManager = WalletManagerPtr(
					new SpvService(_parent->GetID(), _info->GetChainID(), subAccount, subWalletDBPath,
								   _info->GetEarliestPeerTime(), _config, netType));

			_walletManager->RegisterWalletListener(this);
			_walletManager->RegisterPeerManagerListener(this);

			WalletPtr wallet = _walletManager->GetWallet();

			wallet->SetFeePerKb(_config->FeePerKB());
		}

		SubWallet::~SubWallet() {

		}

		std::string SubWallet::GetChainID() const {
			return _info->GetChainID();
		}

		const std::string &SubWallet::GetInfoChainID() const {
			return _info->GetChainID();
		}

		const SubWallet::WalletManagerPtr &SubWallet::GetWalletManager() const {
			return _walletManager;
		}

		nlohmann::json SubWallet::GetBalanceInfo() const {
			ArgInfo("{} {}", _walletManager->GetWallet()->GetWalletID(), GetFunName());

			nlohmann::json info = _walletManager->GetWallet()->GetBalanceInfo();

			ArgInfo("r => {}", info.dump());
			return info;
		}

		std::string SubWallet::GetBalance() const {
			ArgInfo("{} {}", _walletManager->GetWallet()->GetWalletID(), GetFunName());

			std::string balance = _walletManager->GetWallet()->GetBalance(Asset::GetELAAssetID()).getDec();

			ArgInfo("r => {}", balance);

			return balance;
		}

		std::string SubWallet::CreateAddress() {
			ArgInfo("{} {}", _walletManager->GetWallet()->GetWalletID(), GetFunName());

			std::string address = _walletManager->GetWallet()->GetReceiveAddress()->String();

			ArgInfo("r => {}", address);

			return address;
		}

		nlohmann::json SubWallet::GetAllAddress(uint32_t start, uint32_t count, bool internal) const {
			ArgInfo("{} {}", _walletManager->GetWallet()->GetWalletID(), GetFunName());
			ArgInfo("start: {}", start);
			ArgInfo("count: {}", count);

			nlohmann::json j;
			AddressArray addresses;
			size_t maxCount = _walletManager->GetWallet()->GetAllAddresses(addresses, start, count, internal);

			std::vector<std::string> addrString;
			for (size_t i = 0; i < addresses.size(); ++i) {
				addrString.push_back(addresses[i]->String());
			}

			j["Addresses"] = addrString;
			j["MaxCount"] = maxCount;

			ArgInfo("r => {}", j.dump());
			return j;
		}

		nlohmann::json SubWallet::GetAllPublicKeys(uint32_t start, uint32_t count) const {
			ArgInfo("{} {}", _walletManager->GetWallet()->GetWalletID(), GetFunName());
			ArgInfo("start: {}", start);
			ArgInfo("count: {}", count);

			nlohmann::json j;
			std::vector<bytes_t> publicKeys;
			size_t maxCount = _walletManager->GetWallet()->GetAllPublickeys(publicKeys, start, count, false);

			std::vector<std::string> pubKeyString;
			for (size_t i = 0; i < publicKeys.size(); ++i) {
				pubKeyString.push_back(publicKeys[i].getHex());
			}

			j["PublicKeys"] = pubKeyString;
			j["MaxCount"] = maxCount;

			ArgInfo("r => {}", j.dump());
			return j;
		}

		std::string SubWallet::GetBalanceWithAddress(const std::string &address) const {
			ArgInfo("{} {}", _walletManager->GetWallet()->GetWalletID(), GetFunName());
			ArgInfo("addr: {}", address);

			std::string balance = _walletManager->GetWallet()->GetBalanceWithAddress(Asset::GetELAAssetID(), address).getDec();

			ArgInfo("r => {}", balance);
			return balance;
		}

		void SubWallet::AddCallback(ISubWalletCallback *subCallback) {
			ArgInfo("{} {}", _walletManager->GetWallet()->GetWalletID(), GetFunName());
			ArgInfo("callback: *");

			boost::mutex::scoped_lock scoped_lock(lock);

			if (_callback != nullptr) {
				Log::warn("{} callback registered, ignore", _walletManager->GetWallet()->GetWalletID());
			} else {
				_callback = subCallback;

				const PeerManagerPtr &peerManager = _walletManager->GetPeerManager();

				uint32_t currentHeight = peerManager->GetLastBlockHeight();
				uint32_t lastBlockTime = peerManager->GetLastBlockTimestamp();
				uint32_t progress = (uint32_t)(peerManager->GetSyncProgress(0) * 100);

				nlohmann::json j;
				j["Progress"] = progress;
				j["LastBlockTime"] = lastBlockTime;
				j["BytesPerSecond"] = 0;
				j["DownloadPeer"] = "";

				_callback->OnBlockSyncProgress(j);
				ArgInfo("add callback done");
			}
		}

		void SubWallet::RemoveCallback() {
			ArgInfo("{} {}", _walletManager->GetWallet()->GetWalletID(), GetFunName());
			boost::mutex::scoped_lock scoped_lock(lock);

			_callback = nullptr;

			ArgInfo("remove callback done");
		}

		TransactionPtr SubWallet::CreateConsolidateTx(const std::string &memo, const uint256 &asset) const {
			std::string m;

			if (!memo.empty())
				m = "type:text,msg:" + memo;

			TransactionPtr tx = _walletManager->GetWallet()->Consolidate(m, asset);

			if (_info->GetChainID() == "ELA")
				tx->SetVersion(Transaction::TxVersion::V09);

			tx->FixIndex();

			return tx;
		}

		void SubWallet::EncodeTx(nlohmann::json &result, const TransactionPtr &tx) const {
			ByteStream stream;
			tx->Serialize(stream, true);
			const bytes_t &hex = stream.GetBytes();

			result["Algorithm"] = "base64";
			result["ID"] = tx->GetHash().GetHex().substr(0, 8);
			result["Data"] = hex.getBase64();
			result["ChainID"] = GetChainID();
			result["Fee"] = tx->GetFee();
		}

		TransactionPtr SubWallet::DecodeTx(const nlohmann::json &encodedTx) const {
			if (encodedTx.find("Algorithm") == encodedTx.end() ||
				encodedTx.find("Data") == encodedTx.end() ||
				encodedTx.find("ChainID") == encodedTx.end()) {
				ErrorChecker::ThrowParamException(Error::InvalidArgument, "Invalid input");
			}

			std::string algorithm, data, chainID;

			try {
				algorithm = encodedTx["Algorithm"].get<std::string>();
				data = encodedTx["Data"].get<std::string>();
				chainID = encodedTx["ChainID"].get<std::string>();
			} catch (const std::exception &e) {
				ErrorChecker::ThrowParamException(Error::InvalidArgument, "Invalid input: " + std::string(e.what()));
			}

			if (chainID != GetChainID()) {
				ErrorChecker::ThrowParamException(Error::InvalidArgument,
												  "Invalid input: tx is not belongs to current subwallet");
			}

			TransactionPtr tx;
			if (GetChainID() == CHAINID_MAINCHAIN) {
				tx = TransactionPtr(new Transaction());
			} else if (GetChainID() == CHAINID_IDCHAIN || GetChainID() == CHAINID_TOKENCHAIN) {
				tx = TransactionPtr(new IDTransaction());
			}

			bytes_t rawHex;
			if (algorithm == "base64") {
				rawHex.setBase64(data);
			} else {
				ErrorChecker::CheckCondition(true, Error::InvalidArgument, "Decode tx with unknown algorithm");
			}

			ByteStream stream(rawHex);
			ErrorChecker::CheckParam(!tx->Deserialize(stream, true), Error::InvalidArgument,
									 "Invalid input: deserialize fail");

			SPVLOG_DEBUG("tx: {}", tx->ToJson().dump(4));
			return tx;
		}

		nlohmann::json SubWallet::CreateTransaction(const std::string &fromAddress, const std::string &toAddress,
		                                            const std::string &amount, const std::string &memo) {

			WalletPtr wallet = _walletManager->GetWallet();
			ArgInfo("{} {}", wallet->GetWalletID(), GetFunName());
			ArgInfo("fromAddr: {}", fromAddress);
			ArgInfo("toAddr: {}", toAddress);
			ArgInfo("amount: {}", amount);
			ArgInfo("memo: {}", memo);

			ErrorChecker::CheckBigIntAmount(amount);
			bool max = false;
			BigInt bnAmount;
			if (amount == "-1") {
				max = true;
				bnAmount = 0;
			} else {
				bnAmount.setDec(amount);
			}

			OutputArray outputs;
			Address receiveAddr(toAddress);
			outputs.push_back(OutputPtr(new TransactionOutput(bnAmount, receiveAddr)));
			AddressPtr fromAddr(new Address(fromAddress));

			PayloadPtr payload = PayloadPtr(new TransferAsset());
			TransactionPtr tx = wallet->CreateTransaction(Transaction::transferAsset,
																			   payload, fromAddr, outputs, memo, max);

			nlohmann::json result;
			EncodeTx(result, tx);

			ArgInfo("r => {}", result.dump());
			return result;
		}

		nlohmann::json SubWallet::SignTransaction(const nlohmann::json &createdTx,
												  const std::string &payPassword) const {

			ArgInfo("{} {}", _walletManager->GetWallet()->GetWalletID(), GetFunName());
			ArgInfo("tx: {}", createdTx.dump());
			ArgInfo("passwd: *");

			TransactionPtr tx = DecodeTx(createdTx);

			_walletManager->GetWallet()->SignTransaction(tx, payPassword);

			nlohmann::json result;
			EncodeTx(result, tx);

			ArgInfo("r => {}", result.dump());
			return result;
		}

		nlohmann::json SubWallet::PublishTransaction(const nlohmann::json &signedTx) {
			ArgInfo("{} {}", _walletManager->GetWallet()->GetWalletID(), GetFunName());
			ArgInfo("tx: {}", signedTx.dump());

			TransactionPtr tx = DecodeTx(signedTx);

			SPVLOG_DEBUG("publishing tx: {}", tx->ToJson().dump(4));
			publishTransaction(tx);

			nlohmann::json result;
			result["TxHash"] = tx->GetHash().GetHex();
			result["Fee"] = tx->GetFee();

			ArgInfo("r => {}", result.dump());
			return result;
		}

		nlohmann::json SubWallet::GetAllUTXOs(uint32_t start, uint32_t count, const std::string &address) const {
			ArgInfo("{} {}", _walletManager->GetWallet()->GetWalletID(), GetFunName());
			ArgInfo("start: {}", start);
			ArgInfo("count: {}", count);
			ArgInfo("addr: {}", address);
			size_t maxCount = 0, pageCount = 0;

			const WalletPtr &wallet = _walletManager->GetWallet();

			std::vector<UTXOPtr> UTXOs = wallet->GetAllUTXO(address);

			maxCount = UTXOs.size();
			nlohmann::json j, jutxos;

			for (size_t i = start; i < UTXOs.size() && pageCount < count; ++i) {
				nlohmann::json item;
				item["Hash"] = UTXOs[i]->Hash().GetHex();
				item["Index"] = UTXOs[i]->Index();
				item["Amount"] = UTXOs[i]->Output()->Amount().getDec();
				jutxos.push_back(item);
				pageCount++;
			}

			j["MaxCount"] = maxCount;
			j["UTXOs"] = jutxos;

			ArgInfo("r => {}", j.dump());
			return j;
		}

		nlohmann::json SubWallet::CreateConsolidateTransaction(const std::string &memo) {
			ArgInfo("{} {}", _walletManager->GetWallet()->GetWalletID(), GetFunName());
			ArgInfo("memo: {}", memo);

			TransactionPtr tx = CreateConsolidateTx(memo, Asset::GetELAAssetID());

			nlohmann::json result;
			EncodeTx(result, tx);

			ArgInfo("r => {}", result.dump());
			return result;
		}

		nlohmann::json SubWallet::GetAllTransaction(uint32_t start, uint32_t count, const std::string &txid) const {
			ArgInfo("{} {}", _walletManager->GetWallet()->GetWalletID(), GetFunName());
			ArgInfo("start: {}", start);
			ArgInfo("count: {}", count);
			ArgInfo("txid: {}", txid);

			nlohmann::json j;
			uint32_t confirms = 0;
			std::vector<nlohmann::json> jsonList;
			const WalletPtr &wallet = _walletManager->GetWallet();

			j["MaxCount"] = wallet->GetAllTransactionCount();

			if (!txid.empty()) {
				uint256 txHash(txid);
				TransactionPtr tx = wallet->TransactionForHash(txHash);
				if (tx) {
					confirms = tx->GetConfirms(wallet->LastBlockHeight());
					jsonList.push_back(tx->GetSummary(wallet, confirms, true));
					j["Transactions"] = jsonList;
				} else {
					j["Transactions"] = {};
				}
			} else {
				std::vector<TransactionPtr> txns = wallet->GetAllTransactions(start, count);
				for (size_t i = 0; i < txns.size(); ++i) {
					confirms = txns[i]->GetConfirms(wallet->LastBlockHeight());
					jsonList.push_back(txns[i]->GetSummary(wallet, confirms, false));
				}
				j["Transactions"] = jsonList;
			}

			ArgInfo("r => {}", j.dump());
			return j;
		}

		nlohmann::json SubWallet::GetAllCoinBaseTransaction(uint32_t start, uint32_t count,
															const std::string &txID) const {
			ArgInfo("{} {}", _walletManager->GetWallet()->GetWalletID(), GetFunName());
			ArgInfo("start: {}", start);
			ArgInfo("count: {}", count);
			ArgInfo("txID: {}", txID);

			nlohmann::json j;
			const WalletPtr wallet = _walletManager->GetWallet();
			std::vector<UTXOPtr> cbs = wallet->GetAllCoinBaseTransactions();
			size_t maxCount = cbs.size();
			size_t pageCount = count, realCount = 0;

			if (start >= maxCount) {
				j["Transactions"] = {};
				j["MaxCount"] = maxCount;
				ArgInfo("r => {}", j.dump());
				return j;
			}

			if (maxCount < start + count)
				pageCount = maxCount - start;

			if (!txID.empty())
				pageCount = 1;

			std::vector<nlohmann::json> jcbs;
			jcbs.reserve(pageCount);
			for (size_t i = maxCount - start; i > 0 && realCount < pageCount; --i) {
				const UTXOPtr &cbptr = cbs[i - 1];
				nlohmann::json cb;

				if (!txID.empty()) {
					if (cbptr->Hash().GetHex() == txID) {
						cb["TxHash"] = txID;
						uint32_t confirms = cbptr->GetConfirms(_walletManager->GetWallet()->LastBlockHeight());
						cb["Timestamp"] = cbptr->Timestamp();
						cb["Amount"] = cbptr->Output()->Amount().getDec();
						cb["Status"] = confirms <= 100 ? "Pending" : "Confirmed";
						cb["Direction"] = "Received";

						cb["ConfirmStatus"] = confirms <= 100 ? std::to_string(confirms) : "100+";
						cb["Height"] = cbptr->BlockHeight();
						cb["Spent"] = cbptr->Spent();
						cb["Address"] = cbptr->Output()->Addr()->String();
						cb["Type"] = Transaction::coinBase;
						jcbs.push_back(cb);
						realCount++;
						break;
					}
				} else {
					nlohmann::json cb;

					cb["TxHash"] = cbptr->Hash().GetHex();
					uint32_t confirms = cbptr->GetConfirms(_walletManager->GetWallet()->LastBlockHeight());
					cb["Timestamp"] = cbptr->Timestamp();
					cb["Amount"] = cbptr->Output()->Amount().getDec();
					cb["Status"] = confirms <= 100 ? "Pending" : "Confirmed";
					cb["Direction"] = "Received";

					jcbs.push_back(cb);
					realCount++;
				}
			}
			j["Transactions"] = jcbs;
			j["MaxCount"] = maxCount;

			ArgInfo("r => {}", j.dump());
			return j;
		}

		void SubWallet::publishTransaction(const TransactionPtr &tx) {
			_walletManager->PublishTransaction(tx);
		}

		void SubWallet::balanceChanged(const uint256 &assetID, const BigInt &balance) {
			ArgInfo("{} {} Balance: {}", _walletManager->GetWallet()->GetWalletID(), GetFunName(), balance.getDec());
			boost::mutex::scoped_lock scoped_lock(lock);

			if (_callback) {
				_callback->OnBalanceChanged(assetID.GetHex(), balance.getDec());
			} else {
				Log::warn("{} callback not register", _walletManager->GetWallet()->GetWalletID());
			}
		}

		void SubWallet::onCoinBaseTxAdded(const UTXOPtr &cb) {
			ArgInfo("{} {} Hash:{}", _walletManager->GetWallet()->GetWalletID(), GetFunName(), cb->Hash().GetHex());
		}

		void SubWallet::onCoinBaseUpdatedAll(const UTXOArray &cbs) {
			ArgInfo("{} {} cnt: {}", _walletManager->GetWallet()->GetWalletID(), GetFunName(), cbs.size());
		}

		void SubWallet::onCoinBaseTxUpdated(const std::vector<uint256> &hashes, uint32_t blockHeight, time_t timestamp) {
			ArgInfo("{} {} size: {}, height: {}, timestamp: {}",
					   _walletManager->GetWallet()->GetWalletID(), GetFunName(),
					   hashes.size(), blockHeight, timestamp);
		}

		void SubWallet::onCoinBaseSpent(const UTXOArray &spentUTXO) {
			ArgInfo("{} {} size: {}: [{},{} {}]",
					   _walletManager->GetWallet()->GetWalletID(), GetFunName(),
					   spentUTXO.size(), spentUTXO.front()->Hash().GetHex(),
					   (spentUTXO.size() > 2 ? " ...," : ""),
					   (spentUTXO.size() > 1 ? spentUTXO.back()->Hash().GetHex() : ""));
		}

		void SubWallet::onCoinBaseTxDeleted(const uint256 &hash, bool notifyUser, bool recommendRescan) {
			ArgInfo("{} {} Hash: {}, notify: {}, rescan: {}",
					_walletManager->GetWallet()->GetWalletID(), GetFunName(),
					hash.GetHex(), notifyUser, recommendRescan);
		}

		void SubWallet::onTxAdded(const TransactionPtr &tx) {
			const uint256 &txHash = tx->GetHash();
			ArgInfo("{} {} Hash: {}, h: {}", _walletManager->GetWallet()->GetWalletID(), GetFunName(), txHash.GetHex(), tx->GetBlockHeight());

			fireTransactionStatusChanged(txHash, "Added", nlohmann::json(), 0);
		}

		void SubWallet::onTxUpdated(const std::vector<uint256> &hashes, uint32_t blockHeight, time_t timestamp) {
			ArgInfo("{} {} size: {}, height: {}, timestamp: {}", _walletManager->GetWallet()->GetWalletID(),
					GetFunName(),
					hashes.size(), blockHeight, timestamp);

			if (_walletManager->GetAllTransactionsCount() == 1) {
				_info->SetEaliestPeerTime(timestamp);
				_parent->_account->Save();
			}

			for (size_t i = 0; i < hashes.size(); ++i) {
				TransactionPtr tx = _walletManager->GetWallet()->TransactionForHash(hashes[i]);
				uint32_t confirm = tx->GetConfirms(blockHeight);

				fireTransactionStatusChanged(hashes[i], "Updated", nlohmann::json(), confirm);
			}
		}

		void SubWallet::onTxDeleted(const uint256 &hash, bool notifyUser, bool recommendRescan) {
			ArgInfo("{} {} hash: {}, notify: {}, rescan: {}", _walletManager->GetWallet()->GetWalletID(), GetFunName(),
					hash.GetHex(), notifyUser, recommendRescan);

			fireTransactionStatusChanged(hash, "Deleted", nlohmann::json(), 0);
		}

		void SubWallet::onTxUpdatedAll(const std::vector<TransactionPtr> &txns) {
			ArgInfo("{} {} tx cnt: {}", _walletManager->GetWallet()->GetWalletID(), GetFunName(), txns.size());
		}

		void SubWallet::onAssetRegistered(const AssetPtr &asset, uint64_t amount, const uint168 &controller) {
			ArgInfo("{} {} asset: {}, amount: {}",
					_walletManager->GetWallet()->GetWalletID(), GetFunName(),
					asset->GetName(), amount, controller.GetHex());

			boost::mutex::scoped_lock scoped_lock(lock);

			if (_callback) {
				_callback->OnAssetRegistered(asset->GetHash().GetHex(), asset->ToJson());
			} else {
				Log::warn("{} callback not register", _walletManager->GetWallet()->GetWalletID());
			}
		}

		void SubWallet::syncStarted() {
		}

		void SubWallet::syncProgress(uint32_t progress, time_t lastBlockTime, uint32_t bytesPerSecond, const std::string &downloadPeer) {
			struct tm tm;

			localtime_r(&lastBlockTime, &tm);
			char timeString[100] = {0};
			strftime(timeString, sizeof(timeString), "%F %T", &tm);
			ArgInfo("{} {} [{}] [{}] [{}%  {} Bytes / s]", _walletManager->GetWallet()->GetWalletID(), GetFunName(),
					downloadPeer, timeString, progress, bytesPerSecond);

			nlohmann::json j;
			j["Progress"] = progress;
			j["LastBlockTime"] = lastBlockTime;
			j["BytesPerSecond"] = bytesPerSecond;
			j["DownloadPeer"] = downloadPeer;

			boost::mutex::scoped_lock scoped_lock(lock);

			if (_callback) {
				_callback->OnBlockSyncProgress(j);
			} else {
				Log::warn("{} callback not register", _walletManager->GetWallet()->GetWalletID());
			}
		}

		void SubWallet::syncStopped(const std::string &error) {
		}

		void SubWallet::saveBlocks(bool replace, const std::vector<MerkleBlockPtr> &blocks) {
		}

		void SubWallet::txPublished(const std::string &hash, const nlohmann::json &result) {
			ArgInfo("{} {} hash: {} result: {}", _walletManager->GetWallet()->GetWalletID(), GetFunName(), hash, result.dump());

			boost::mutex::scoped_lock scoped_lock(lock);

			if (_callback) {
				_callback->OnTxPublished(hash, result);
			} else {
				Log::warn("{} callback not register", _walletManager->GetWallet()->GetWalletID());
			}
		}

		void SubWallet::connectStatusChanged(const std::string &status) {
			ArgInfo("{} {} status: {}", _walletManager->GetWallet()->GetWalletID(), GetFunName(), status);

			boost::mutex::scoped_lock scopedLock(lock);

			if (_callback) {
				_callback->OnConnectStatusChanged(status);
			} else {
				Log::warn("{} callback not register", _walletManager->GetWallet()->GetWalletID());
			}
		}

		void SubWallet::fireTransactionStatusChanged(const uint256 &txid, const std::string &status,
													 const nlohmann::json &desc, uint32_t confirms) {
			boost::mutex::scoped_lock scoped_lock(lock);

			if (_callback) {
				_callback->OnTransactionStatusChanged(txid.GetHex(), status, desc, confirms);
			} else {
				Log::warn("{} callback not register", _walletManager->GetWallet()->GetWalletID());
			}
		}

		const CoinInfoPtr &SubWallet::GetCoinInfo() const {
			return _info;
		}

		void SubWallet::StartP2P() {
			_walletManager->SyncStart();
		}

		void SubWallet::StopP2P() {
			_walletManager->SyncStop();
			_walletManager->ExecutorStop();
		}

		void SubWallet::FlushData() {
			_walletManager->DatabaseFlush();
		}

		bool SubWallet::SetFixedPeer(const std::string &address, uint16_t port) {
			ArgInfo("{} {}", _walletManager->GetWallet()->GetWalletID(), GetFunName());
			ArgInfo("address: {}", address);
			ArgInfo("port: {}", port);
			return _walletManager->GetPeerManager()->SetFixedPeer(address, port);
		}

		void SubWallet::SyncStart() {
			ArgInfo("{} {}", _walletManager->GetWallet()->GetWalletID(), GetFunName());
			_walletManager->SyncStart();
		}

		void SubWallet::SyncStop() {
			ArgInfo("{} {}", _walletManager->GetWallet()->GetWalletID(), GetFunName());
			_walletManager->SyncStop();
		}

		nlohmann::json SubWallet::GetBasicInfo() const {
			ArgInfo("{} {}", _walletManager->GetWallet()->GetWalletID(), GetFunName());

			nlohmann::json j;
			j["Info"] = _walletManager->GetWallet()->GetBasicInfo();
			j["ChainID"] = _info->GetChainID();

			ArgInfo("r => {}", j.dump());
			return j;
		}

		nlohmann::json SubWallet::GetTransactionSignedInfo(const nlohmann::json &encodedTx) const {
			ArgInfo("{} {}", _walletManager->GetWallet()->GetWalletID(), GetFunName());
			ArgInfo("tx: {}", encodedTx.dump());

			TransactionPtr tx = DecodeTx(encodedTx);

			nlohmann::json info = tx->GetSignedInfo();

			ArgInfo("r => {}", info.dump());

			return info;
		}

		nlohmann::json SubWallet::GetAssetInfo(const std::string &assetID) const {
			ArgInfo("{} {}", _walletManager->GetWallet()->GetWalletID(), GetFunName());
			ArgInfo("assetID: {}", assetID);

			nlohmann::json info;

			AssetPtr asset = _walletManager->GetWallet()->GetAsset(uint256(assetID));
			info["Registered"] = (asset != nullptr);
			if (asset != nullptr)
				info["Info"] = asset->ToJson();
			else
				info["Info"] = {};

			ArgInfo("r => {}", info.dump());
			return info;
		}

	}
}
