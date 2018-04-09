// Copyright (c) 2012-2018 The Elastos Open Source Project
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "PeerManager.h"

namespace Elastos {
    namespace SDK {

        PeerManager::PeerManager(ChainParams &params,
                                 const boost::shared_ptr<Wallet> &wallet,
                                 uint32_t earliestKeyTime,
                                 const SharedWrapperList<MerkleBlock, BRMerkleBlock *> &blocks,
                                 WrapperList<Peer, BRPeer> &peers,
                                 const boost::shared_ptr<PeerManager::Listener> &listener) {
            _listener = boost::weak_ptr<Listener>(listener);

            BRMerkleBlock **temp;
            _manager = BRPeerManagerNew(
                    params.getRaw(),
                    wallet->getRaw(),
                    earliestKeyTime,
                    blocks.getRawPointerArray().data(),
                    blocks.size(),
                    peers.getRawArray().data(),
                    peers.size()
                    );
        }

        PeerManager::~PeerManager() {
            BRPeerManagerFree(_manager);
        }

        Peer::ConnectStatus PeerManager::getConnectStatus() const {
            //todo complete me
            return Peer::Unknown;
        }
    }
}