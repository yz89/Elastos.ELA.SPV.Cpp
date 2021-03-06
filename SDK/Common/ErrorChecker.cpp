// Copyright (c) 2012-2018 The Elastos Open Source Project
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "Log.h"
#include "ErrorChecker.h"

#include <regex>

namespace Elastos {
	namespace ElaWallet {

		nlohmann::json ErrorChecker::MakeErrorJson(Error::Code err, const std::string &msg) {
			nlohmann::json j;
			j["Code"] = err;
			j["Message"] = msg;
			return j;
		}

		nlohmann::json ErrorChecker::MakeErrorJson(Error::Code err, const std::string &msg, const BigInt &data) {
			nlohmann::json j;
			j["Code"] = err;
			j["Message"] = msg;
			j["Data"] = data.getDec();
			return j;
		}

		void ErrorChecker::ThrowParamException(Error::Code err, const std::string &msg) {
			CheckParam(true, err, msg);
		}

		void ErrorChecker::ThrowLogicException(Error::Code err, const std::string &msg) {
			CheckLogic(true, err, msg);
		}

		void ErrorChecker::CheckParam(bool condition, Error::Code err, const std::string &msg) {
			CheckCondition(condition, err, msg, Exception::Type::InvalidArgument);
		}

		void ErrorChecker::CheckBigIntAmount(const std::string &amount) {
			if (amount == "-1")
				return;

			for (size_t i = 0; i < amount.size(); ++i)
				CheckCondition(!isdigit(amount[i]), Error::InvalidArgument, "invalid bigint amount: " + amount);
		}

		void ErrorChecker::CheckLogic(bool condition, Error::Code err, const std::string &msg) {
			CheckCondition(condition, err, msg, Exception::Type::LogicError);
		}

		void ErrorChecker::CheckCondition(bool condition, Error::Code err, const std::string &msg,
		                                  Exception::Type type, bool enableLog) {
			if (condition) {
				nlohmann::json errJson = MakeErrorJson(err, msg);

				if (enableLog)
					Log::error(errJson.dump());

				if (type == Exception::LogicError) {
					throw std::logic_error(errJson.dump());
				} else if (type == Exception::InvalidArgument) {
					throw std::invalid_argument(errJson.dump());
				}
			}
		}

		void ErrorChecker::CheckPassword(const std::string &password, const std::string &msg) {
			CheckCondition(password.size() < MIN_PASSWORD_LENGTH, Error::InvalidPasswd,
			               msg + " password invalid: less than " + std::to_string(MIN_PASSWORD_LENGTH),
			               Exception::InvalidArgument);

			CheckCondition(password.size() > MAX_PASSWORD_LENGTH, Error::InvalidPasswd,
			               msg + " password invalid: more than " + std::to_string(MAX_PASSWORD_LENGTH),
			               Exception::InvalidArgument);
		}

		void ErrorChecker::CheckPasswordWithNullLegal(const std::string &password, const std::string &msg) {
			if (password.empty())
				return;

			CheckPassword(password, msg);
		}

		void ErrorChecker::CheckParamNotEmpty(const std::string &argument, const std::string &msg) {
			CheckCondition(argument.empty(), Error::InvalidArgument, msg + " should not be empty",
			               Exception::InvalidArgument);
		}

		void ErrorChecker::CheckJsonArray(const nlohmann::json &jsonData, size_t count, const std::string &msg) {
			CheckCondition(!jsonData.is_array(), Error::JsonArrayError, msg + " is not json array",
			               Exception::LogicError);
			CheckCondition(jsonData.size() < count, Error::JsonArrayError,
			               msg + " json array size expect at least " + std::to_string(count), Exception::LogicError);
		}

		void ErrorChecker::CheckPathExists(const boost::filesystem::path &path, bool enableLog) {
			CheckCondition(!boost::filesystem::exists(path), Error::PathNotExist,
			               "Path '" + path.string() + "' do not exist", Exception::LogicError, enableLog);
		}

		void ErrorChecker::CheckPrivateKey(const std::string &key) {
			// TODO fix here later
			ErrorChecker::CheckCondition(key.find("xprv") != -1, Error::InvalidArgument,
			                             "Private key is not support xprv");

			ErrorChecker::CheckCondition(key.length() != 32 * 2, Error::InvalidArgument,
			                             "Private key length should be 32 bytes");
		}

		void ErrorChecker::CheckInternetDate(const std::string &date) {
			std::regex reg = std::regex("(\\d{4})-(0\\d{1}|1[0-2])-(0\\d{1}|[12]\\d{1}|3[01])T(0\\d{1}|1\\d{1}|2[0-3]):[0-5]\\d{1}:([0-5]\\d{1}Z)");
			ErrorChecker::CheckParam(!std::regex_match(date, reg), Error::InvalidArgument,
			                         "date format is error. such as 2019-01-01T19:20:18Z");
		}

	}
}

