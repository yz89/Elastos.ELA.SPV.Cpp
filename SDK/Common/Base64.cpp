// Copyright (c) 2012-2018 The Elastos Open Source Project
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "Base64.h"

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

namespace Elastos {
	namespace ElaWallet {

		std::string Base64::Encode(const void *input, size_t inputLen) {
			BIO *bio, *b64;
			BUF_MEM *bufferPtr;
			b64 = BIO_new(BIO_f_base64());
			bio = BIO_new(BIO_s_mem());
			bio = BIO_push(b64, bio);

			BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
			BIO_write(bio, input, inputLen);
			BIO_flush(bio);
			BIO_get_mem_ptr(bio, &bufferPtr);
			std::string result(bufferPtr->data, bufferPtr->length);
			BIO_set_close(bio, BIO_CLOSE);
			BIO_free_all(bio);
			return result;
		}

		std::string Base64::Encode(const bytes_t &input) {
			return Encode(input.data(), input.size());
		}

		bytes_t Base64::Decode(const std::string &input) {
			size_t len = input.size(), padding = 0;
			if (input[len - 1] == '=' && input[len - 2] == '=') //last two chars are =
				padding = 2;
			else if (input[len - 1] == '=') //last char is =
				padding = 1;

			int decodedLen = (len*3)/4 - padding;

			BIO *bio, *b64;

			bytes_t buffer(decodedLen);

			bio = BIO_new_mem_buf((void *)input.c_str(), -1);
			b64 = BIO_new(BIO_f_base64());
			bio = BIO_push(b64, bio);

			BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
			decodedLen = BIO_read(bio, &buffer[0], input.size());
			BIO_free_all(bio);
			buffer.resize(decodedLen);
			return buffer;
		}

		std::string Base64::EncodeURL(const bytes_t &input) {
			std::string str = Encode(input);
			str.erase(std::remove(str.begin(), str.end(), '='), str.end());
			std::replace(str.begin(), str.end(), '+', '-');
			std::replace(str.begin(), str.end(), '/', '_');
			return str;
		}

		bytes_t Base64::DecodeURL(const std::string &input) {
			std::string str = input;
			std::replace(str.begin(), str.end(), '-', '+');
			std::replace(str.begin(), str.end(), '_', '/');

			long append = 4 - str.length() % 4;
			if (append != 4)
				str += std::string(append, '=');

			return Decode(str);
		}

	}
}