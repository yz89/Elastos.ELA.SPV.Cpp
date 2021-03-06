// Copyright (c) 2012-2018 The Elastos Open Source Project
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef __ELASTOS_SDK_TRANSACTIONSET_H__
#define __ELASTOS_SDK_TRANSACTIONSET_H__

#include <set>
#include <Common/uint256.h>

namespace Elastos {
	namespace ElaWallet {

		template<class T>
		class ElementSet {
		public:
			typedef struct {
				bool operator() (const T &x, const T &y) const {
					return x->GetHash() < y->GetHash();
				}
			} TCompare;

			T Get(const uint256 &hash) const {
				typename std::set<T, TCompare>::const_iterator it;
				it = std::find_if(_elements.begin(), _elements.end(), [&hash](const T &e) {
					return hash == e->GetHash();
				});

				if (it == _elements.end())
					return nullptr;

				return *it;
			}

			std::set<T, TCompare> &Raw() {
				return _elements;
			}

			bool Contains(const T &e) const {
				return _elements.find(e) != _elements.end();
			}

			bool Contains(const uint256 &hash) const {
				return Get(hash) != nullptr;
			}

			bool Insert(const T &e) {
				return _elements.insert(e).second;
			}

			size_t Size() {
				return _elements.size();
			}

			bool Remove(const T &e) {
				return _elements.erase(e) > 0;
			}

			bool RemoveMatchPrevHash(const uint256 &hash) {
				typename std::set<T, TCompare>::const_iterator it;
				it = std::find_if(_elements.cbegin(), _elements.cend(), [&hash](const T &e) {
					return hash == e->GetPrevBlockHash();
				});

				if (it != _elements.end()) {
					_elements.erase(it);
					return true;
				}

				return false;
			}

			T GetMatchPrevHash(const uint256 &hash) const {
				typename std::set<T, TCompare>::const_iterator it;
				it = std::find_if(_elements.cbegin(), _elements.cend(), [&hash](const T &e) {
					return hash == e->GetPrevBlockHash();
				});

				if (it != _elements.end())
					return *it;

				return nullptr;
			}

			void Clear() {
				_elements.clear();
			}

		private:
			std::set<T, TCompare> _elements;
		};

	}
}

#endif //__ELASTOS_SDK_TRANSACTIONSET_H__
