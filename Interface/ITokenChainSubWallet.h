// Copyright (c) 2012-2019 The Elastos Open Source Project
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "ISidechainSubWallet.h"

#ifndef __ELASTOS_SDK_ITOKENCHAINSUBWALLET_H__
#define __ELASTOS_SDK_ITOKENCHAINSUBWALLET_H__

namespace Elastos {
	namespace ElaWallet {

		class ITokenchainSubWallet: public virtual ISidechainSubWallet {
		public:
			/**
			 * Get balances of all addresses in json format.
			 * @param assetID asset hex code from asset hash.
			 * @return balances of all addresses in json format.
			 */
			virtual nlohmann::json GetBalanceInfo(const std::string &assetID) const = 0;

			/**
			 * Get sum of balances of all addresses.
			 * @param assetID asset hex code from asset hash.
			 * @return sum of balances in big int string. 1 Token = "1.0000"
			 */
			virtual std::string GetBalance(const std::string &assetID) const = 0;

			/**
			 * Get balance of only the specified address.
			 * @param assetID asset hex code from asset hash.
			 * @param address is one of addresses created by current sub wallet.
			 * @return balance of specified address in big int string.
			 */
			virtual std::string GetBalanceWithAddress(const std::string &assetID, const std::string &address) const = 0;

			/**
			 * Register asset.
			 * @param name Asset name such as ELA, IDChain.
			 * @param description
			 * @param address Address where to receive registered asset amount.
			 * @param registerAmount Integer part amount.
			 * @param precision Range: [0, 18].
			 * @param memo
			 * @param remark
			 * @return Transaction in json format.
			 */
			virtual nlohmann::json CreateRegisterAssetTransaction(
				const std::string &name,
				const std::string &description,
				const std::string &registerToAddress,
				uint64_t registerAmount,
				uint8_t precision,
				const std::string &memo,
				const std::string &remark) = 0;

			/**
			 * Create a normal transaction and return the content of transaction in json format.
			 * @param fromAddress specify which address we want to spend, or just input empty string to let wallet choose UTXOs automatically.
			 * @param toAddress specify which address we want to send.
			 * @param amount specify amount(big int string) we want to send.
			 * @param assetID specify asset ID
			 * @param memo input memo attribute for describing.
			 * @param remark is used to record message of local wallet.
			 * @return If success return the content of transaction in json format.
			 */
			virtual nlohmann::json CreateTransaction(
				const std::string &fromAddress,
				const std::string &toAddress,
				const std::string &amount,
				const std::string &assetID,
				const std::string &memo,
				const std::string &remark) = 0;

			/**
			 * Get all visible assets in json format. Note this is a sub set of supported assets.
			 * @return assets list in json format
			 */
			virtual nlohmann::json GetAllVisibleAssets() const = 0;

			/**
			 * Set visible assets by specify a json that contains a list of asset ID.
			 * @param assets a json contains a list of asset ID
			 */
			virtual void SetVisibleAssets(const nlohmann::json &assets) = 0;

			/**
			 * Set a single asset to be visible.
			 * @param asset ID.
			 */
			virtual void SetVisibleAsset(const std::string &asset) = 0;

			/**
			 * Get all supported assets in json format
			 * @return assets list in json format
			 */
			virtual nlohmann::json GetAllSupportedAssets() const = 0;

		};

	}
}

#endif //__ELASTOS_SDK_ITOKENCHAINSUBWALLET_H__
