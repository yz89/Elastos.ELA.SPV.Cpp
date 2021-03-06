// Copyright (c) 2012-2018 The Elastos Open Source Project
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "TableBase.h"

#include <Common/Log.h>

namespace Elastos {
	namespace ElaWallet {

		TableBase::TableBase(Sqlite *sqlite) :
				_sqlite(sqlite),
				_txType(IMMEDIATE) {
		}

		TableBase::TableBase(SqliteTransactionType type, Sqlite *sqlite) :
				_sqlite(sqlite),
				_txType(type) {
		}

		TableBase::~TableBase() {

		}

		bool TableBase::DoTransaction(const boost::function<bool()> &fun) const {

			bool result;
			_sqlite->BeginTransaction(_txType);
			try {
				result = fun();
			} catch (const std::exception &e) {
				result = false;
				Log::error("Data base error: {}", e.what());
			} catch (...) {
				result = false;
				Log::error("Unknown data base error.");
			}
			_sqlite->EndTransaction();

			return result;
		}

		void TableBase::InitializeTable(const std::string &constructScript) {
			_sqlite->BeginTransaction(_txType);
			_sqlite->exec(constructScript, nullptr, nullptr);
			_sqlite->EndTransaction();
		}
	}
}