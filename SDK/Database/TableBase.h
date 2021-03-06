// Copyright (c) 2012-2018 The Elastos Open Source Project
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef __ELASTOS_SDK_TABLEBASE_H__
#define __ELASTOS_SDK_TABLEBASE_H__

#include "Sqlite.h"
#include <CMakeConfig.h>

#include <boost/function.hpp>


namespace Elastos {
	namespace ElaWallet {

		class TableBase {
		public:
			TableBase(Sqlite *sqlite);

			TableBase(SqliteTransactionType type, Sqlite *sqlite);

			virtual ~TableBase();

		protected:
			void InitializeTable(const std::string &constructScript);

			bool DoTransaction(const boost::function<bool()> &fun) const;

		protected:
			Sqlite *_sqlite;
			SqliteTransactionType _txType;
		};

	}
}

#endif //__ELASTOS_SDK_TABLEBASE_H__
