// Copyright (c) 2012-2019 The Elastos Open Source Project
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "CoinBaseUTXODataStore.h"

#include <Common/ErrorChecker.h>
#include <Common/Log.h>
#include <Wallet/UTXO.h>
#include <WalletCore/Address.h>
#include <Plugin/Transaction/TransactionOutput.h>
#include <cstdint>

namespace Elastos {
	namespace ElaWallet {

		CoinBaseUTXODataStore::CoinBaseUTXODataStore(Sqlite *sqlite) :
			TableBase(sqlite) {
			InitializeTable(_databaseCreate);
		}

		CoinBaseUTXODataStore::~CoinBaseUTXODataStore() {

		}

		bool CoinBaseUTXODataStore::Put(const std::vector<UTXOPtr> &entitys) {
			if (entitys.empty())
				return true;

			return DoTransaction([&entitys, this]() {
				for (size_t i = 0; i < entitys.size(); ++i) {
					if (!this->PutInternal(entitys[i]))
						return false;
				}

				return true;
			});
		}

		bool CoinBaseUTXODataStore::Put(const UTXOPtr &entity) {
			return DoTransaction([&entity, this]() {
				return this->PutInternal(entity);
			});
		}

		bool CoinBaseUTXODataStore::DeleteAll() {
			return DoTransaction([this]() {
				std::string sql;

				sql = "DELETE FROM " + _tableName + ";";

				if (!_sqlite->exec(sql, nullptr, nullptr)) {
					Log::error("Exec sql: {}", sql);
					return false;
				}

				return true;
			});
		}

		size_t CoinBaseUTXODataStore::GetTotalCount() const {
			size_t count = 0;

			DoTransaction([&count, this]() {
				std::string sql;

				sql = "SELECT COUNT(" + _txHash + ") AS nums FROM " + _tableName + ";";

				sqlite3_stmt *stmt;
				if (!_sqlite->Prepare(sql, &stmt, nullptr)) {
					Log::error("prepare sql: {}", sql);
					return false;
				}

				if (SQLITE_ROW == _sqlite->Step(stmt)) {
					count = (uint32_t) _sqlite->ColumnInt(stmt, 0);
				}

				if (!_sqlite->Finalize(stmt)) {
					Log::error("Coinbase get count finalize");
					return false;
				}

				return true;
			});

			return count;
		}

		std::vector<UTXOPtr> CoinBaseUTXODataStore::GetAll() const {
			std::vector<UTXOPtr> entitys;

			std::string sql;

			sql = "SELECT " + _txHash + ", " + _blockHeight + ", " + _timestamp + ", " + _index + ", " +
				  _programHash + ", " + _assetID + ", " + _outputLock + ", " + _amount + ", " +
				  _payload + ", " + _spent + " FROM " + _tableName + /*" ORDER BY " + _blockHeight + " ASC*/";";

			sqlite3_stmt *stmt;
			if (!_sqlite->Prepare(sql, &stmt, nullptr)) {
				Log::error("prepare sql: {}", sql);
				return {};
			}

			while (SQLITE_ROW == _sqlite->Step(stmt)) {
				uint256 txHash(_sqlite->ColumnText(stmt, 0));
				uint32_t blockHeight = _sqlite->ColumnInt(stmt, 1);
				time_t timestamp = _sqlite->ColumnInt64(stmt, 2);
				uint16_t index = (uint16_t) _sqlite->ColumnInt(stmt, 3);
				uint168 programHash(*_sqlite->ColumnBlobBytes(stmt, 4));
				uint256 assetID(*_sqlite->ColumnBlobBytes(stmt, 5));
				uint32_t outputLock = _sqlite->ColumnInt(stmt, 6);
				BigInt amount;
				amount.setDec(_sqlite->ColumnText(stmt, 7));
				_sqlite->ColumnBlobBytes(stmt, 8);
				bool spent = _sqlite->ColumnInt(stmt, 9) != 0;

				OutputPtr o(new TransactionOutput(amount, Address(programHash), assetID));
				o->SetOutputLock(outputLock);
				o->SetFixedIndex(index);

				UTXOPtr entity(new UTXO(txHash, index, timestamp, blockHeight, o));
				entity->SetSpent(spent);
				entitys.push_back(entity);
			}

			if (!_sqlite->Finalize(stmt)) {
				Log::error("Coinbase get all finalize");
				return {};
			}

			return entitys;
		}

		bool CoinBaseUTXODataStore::Update(const std::vector<uint256> &txHashes, uint32_t blockHeight,
										   time_t timestamp) {
			if (txHashes.empty())
				return true;

			return DoTransaction([&txHashes, &blockHeight, &timestamp, this]() {
				std::string sql, hash;

				for (size_t i = 0; i < txHashes.size(); ++i) {
					hash = txHashes[i].GetHex();
					sql = "UPDATE " + _tableName + " SET " + _blockHeight + " = ?, " +
						  _timestamp + " = ? WHERE " + _txHash + " = ?;";

					sqlite3_stmt *stmt;
					if (!_sqlite->Prepare(sql, &stmt, nullptr)) {
						Log::error("prepare sql: {}", sql);
						return false;
					}

					if (!_sqlite->BindInt(stmt, 1, blockHeight) ||
						!_sqlite->BindInt64(stmt, 2, timestamp) ||
						!_sqlite->BindText(stmt, 3, hash, nullptr)) {
						Log::error("bind args");
					}

					if (SQLITE_DONE != _sqlite->Step(stmt)) {
						Log::error("step");
					}

					if (!_sqlite->Finalize(stmt)) {
						Log::error("Coinbase update finalize");
						return false;
					}
				}
				return true;
			});
		}

		bool CoinBaseUTXODataStore::UpdateSpent(const UTXOArray &spentUTXO) {
			if (spentUTXO.empty())
				return true;

			return DoTransaction([&spentUTXO, this]() {
				std::string sql, hash;

				for (size_t i = 0; i < spentUTXO.size(); ++i) {
					hash = spentUTXO[i]->Hash().GetHex();
					uint16_t index = spentUTXO[i]->Index();
					sql = "UPDATE " + _tableName + " SET " + _spent + " = ? WHERE " + _txHash + " = ? AND " + _index + " = ?;";

					sqlite3_stmt *stmt;
					if (!_sqlite->Prepare(sql, &stmt, nullptr)) {
						Log::error("prepare sql: {}", sql);
						return false;
					}

					if (!_sqlite->BindInt(stmt, 1, spentUTXO[i]->Spent() ? 1 : 0) ||
						!_sqlite->BindText(stmt, 2, hash, nullptr) ||
						!_sqlite->BindInt(stmt, 3, index)) {
						Log::error("bind args");
					}

					if (SQLITE_DONE != _sqlite->Step(stmt)) {
						Log::error("step");
					}

					if (!_sqlite->Finalize(stmt)) {
						Log::error("Coinbase update spent finalize");
						return false;
					}
				}
				return true;
			});
		}

		bool CoinBaseUTXODataStore::Delete(const uint256 &hash) {
			return DoTransaction([&hash, this]() {
				std::string sql, hashString = hash.GetHex();

				sql = "DELETE FROM " + _tableName + " WHERE " + _txHash + " = ?;";

				sqlite3_stmt *stmt;
				if (!_sqlite->Prepare(sql, &stmt, nullptr)) {
					Log::error("prepare sql: {}", sql);
					return false;
				}

				if (!_sqlite->BindText(stmt, 1, hashString, nullptr)) {
					Log::error("bind args");
				}

				if (SQLITE_DONE != _sqlite->Step(stmt)) {
					Log::error("step");
				}

				if (!_sqlite->Finalize(stmt)) {
					Log::error("Coinbase delete finalize");
					return false;
				}

				return true;
			});
		}

		void CoinBaseUTXODataStore::flush() {
			_sqlite->flush();
		}

		bool CoinBaseUTXODataStore::PutInternal(const UTXOPtr &entity) {
			std::string sql, hash = entity->Hash().GetHex();

			sql = "INSERT INTO " + _tableName + "(" + _txHash + "," + _blockHeight + "," + _timestamp + "," +
				  _index + "," + _programHash + "," + _assetID + "," + _outputLock + "," + _amount + "," +
				  _payload + "," + _spent + ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

			sqlite3_stmt *stmt;
			if (!_sqlite->Prepare(sql, &stmt, nullptr)) {
				Log::error("prepare sql: {}", sql);
				return false;
			}

			if (!_sqlite->BindText(stmt, 1, hash, nullptr) ||
				!_sqlite->BindInt(stmt, 2, entity->BlockHeight()) ||
				!_sqlite->BindInt64(stmt, 3, entity->Timestamp()) ||
				!_sqlite->BindInt(stmt, 4, entity->Index()) ||
				!_sqlite->BindBlob(stmt, 5, entity->Output()->Addr()->ProgramHash().bytes(), nullptr) ||
				!_sqlite->BindBlob(stmt, 6, entity->Output()->AssetID().begin(), entity->Output()->AssetID().size(),
								   nullptr) ||
				!_sqlite->BindInt(stmt, 7, entity->Output()->OutputLock()) ||
				!_sqlite->BindText(stmt, 8, entity->Output()->Amount().getDec(), nullptr) ||
				!_sqlite->BindBlob(stmt, 9, nullptr, 0, nullptr) ||
				!_sqlite->BindInt(stmt, 10, entity->Spent())) {
				Log::error("bind args");
			}

			if (SQLITE_DONE != _sqlite->Step(stmt)) {
				Log::error("step");
			}

			if (!_sqlite->Finalize(stmt)) {
				Log::error("Coinbase put finalize");
				return false;
			}

			return true;
		}

	}
}