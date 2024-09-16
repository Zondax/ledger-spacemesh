/*******************************************************************************
 *   (c) 2018 - 2024 Zondax AG
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 ********************************************************************************/

#include <hexutils.h>
#include <json/json.h>
#include <parser_txdef.h>

#include <fstream>
#include <iostream>

#include "app_mode.h"
#include "crypto.h"
#include "gmock/gmock.h"
#include "parser.h"
#include "parser_message.h"
#include "utils/common.h"

using ::testing::TestWithParam;

typedef struct {
    uint64_t index;
    std::string name;
    bool mainnet;
    std::string blob;
    std::vector<std::string> expected;
    std::vector<std::string> expected_expert;
} testcase_t;

class JsonTestsA : public ::testing::TestWithParam<testcase_t> {
   public:
    struct PrintToStringParamName {
        template <class ParamType>
        std::string operator()(const testing::TestParamInfo<ParamType> &info) const {
            auto p = static_cast<testcase_t>(info.param);
            std::stringstream ss;
            ss << p.index << "_" << p.name;
            return ss.str();
        }
    };
};

class JsonTestsB : public ::testing::TestWithParam<testcase_t> {
   public:
    struct PrintToStringParamName {
        template <class ParamType>
        std::string operator()(const testing::TestParamInfo<ParamType> &info) const {
            auto p = static_cast<testcase_t>(info.param);
            std::stringstream ss;
            ss << p.index << "_" << p.name;
            return ss.str();
        }
    };
};

std::string mainGenesisId = "9eebff023abb17ccb775c602daade8ed708f0a50";
std::string testGenesisId = "e956eff99be943fb70bd385dd509a3f84e9a75dd";

// Retrieve testcases from json file
std::vector<testcase_t> GetJsonTestCases(std::string jsonFile) {
    auto answer = std::vector<testcase_t>();

    Json::CharReaderBuilder builder;
    Json::Value obj;

    std::string fullPathJsonFile = std::string(TESTVECTORS_DIR) + jsonFile;

    std::ifstream inFile(fullPathJsonFile);
    if (!inFile.is_open()) {
        return answer;
    }

    // Retrieve all test cases
    JSONCPP_STRING errs;
    Json::parseFromStream(builder, inFile, &obj, &errs);
    std::cout << "Number of testcases: " << obj.size() << std::endl;

    for (int i = 0; i < obj.size(); i++) {
        auto outputs = std::vector<std::string>();
        for (auto s : obj[i]["output"]) {
            outputs.push_back(s.asString());
        }

        auto outputs_expert = std::vector<std::string>();
        for (auto s : obj[i]["output_expert"]) {
            outputs_expert.push_back(s.asString());
        }

        answer.push_back(testcase_t{obj[i]["index"].asUInt64(), obj[i]["name"].asString(), obj[i]["mainnet"].asBool(),
                                    obj[i]["blob"].asString(), outputs, outputs_expert});
    }

    return answer;
}

void check_testcase(const testcase_t &tc, bool expert_mode) {
    app_mode_set_expert(expert_mode);

    parser_context_t ctx;
    parser_error_t err;

    uint8_t buffer[5000];
    uint16_t bufferLen = parseHexString(buffer, sizeof(buffer), tc.blob.c_str());

    parser_tx_t tx_obj;
    memset(&tx_obj, 0, sizeof(tx_obj));

    hdPath[0] = HDPATH_0_DEFAULT;
    hdPath[1] = tc.mainnet ? HDPATH_1_DEFAULT : HDPATH_1_TESTNET;
    parseHexString(genesisId, sizeof(genesisId), tc.mainnet ? mainGenesisId.c_str() : testGenesisId.c_str());

    err = parser_parse(&ctx, buffer, bufferLen, &tx_obj);
    ASSERT_EQ(err, parser_ok) << parser_getErrorDescription(err);

    auto output = dumpUI(&ctx, 39, 39);

    std::cout << std::endl;
    for (const auto &i : output) {
        std::cout << i << std::endl;
    }
    std::cout << std::endl << std::endl;

    std::vector<std::string> expected = app_mode_expert() ? tc.expected_expert : tc.expected;

    EXPECT_EQ(output.size(), expected.size());
    for (size_t i = 0; i < expected.size(); i++) {
        if (i < output.size()) {
            EXPECT_THAT(output[i], testing::Eq(expected[i]));
        }
    }
}

void check_message_testcase(const testcase_t &tc) {
    app_mode_set_expert(false);

    parser_context_t ctx;
    parser_error_t err;

    uint8_t buffer[5000];
    uint16_t bufferLen = parseHexString(buffer, sizeof(buffer), tc.blob.c_str());

    parser_message_tx_t tx_obj;
    memset(&tx_obj, 0, sizeof(tx_obj));

    hdPath[0] = HDPATH_0_DEFAULT;
    hdPath[1] = HDPATH_1_DEFAULT;

    err = parser_message_parse(&ctx, buffer, bufferLen, &tx_obj);
    ASSERT_EQ(err, parser_ok) << parser_getErrorDescription(err);

    auto output = dumpRawUI(&ctx, 39, 39);

    std::cout << std::endl;
    for (const auto &i : output) {
        std::cout << i << std::endl;
    }
    std::cout << std::endl << std::endl;

    std::vector<std::string> expected = app_mode_expert() ? tc.expected_expert : tc.expected;

    EXPECT_EQ(output.size(), expected.size());
    for (size_t i = 0; i < expected.size(); i++) {
        if (i < output.size()) {
            EXPECT_THAT(output[i], testing::Eq(expected[i]));
        }
    }
}

INSTANTIATE_TEST_SUITE_P

    (JsonTestCasesCurrentTxVer, JsonTestsA, ::testing::ValuesIn(GetJsonTestCases("testcases.json")),
     JsonTestsA::PrintToStringParamName());

INSTANTIATE_TEST_SUITE_P

    (JsonTestCasesRawTxVer, JsonTestsB, ::testing::ValuesIn(GetJsonTestCases("message_testcases.json")),
     JsonTestsB::PrintToStringParamName());

TEST_P(JsonTestsA, CheckUIOutput_CurrentTX_Expert) { check_testcase(GetParam(), true); }
TEST_P(JsonTestsA, CheckUIOutput_CurrentTX) { check_testcase(GetParam(), false); }

TEST_P(JsonTestsB, CheckUIOutput_RawTX) { check_message_testcase(GetParam()); }
