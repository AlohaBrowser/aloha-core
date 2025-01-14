/*
 * This file is part of eyeo Chromium SDK,
 * Copyright (C) 2006-present eyeo GmbH
 *
 * eyeo Chromium SDK is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * eyeo Chromium SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with eyeo Chromium SDK.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "components/adblock/core/subscription/subscription_validator_impl.h"

#include "base/base64.h"
#include "base/files/file_path.h"
#include "base/test/task_environment.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/common/adblock_prefs.h"
#include "components/adblock/core/common/adblock_utils.h"
#include "components/adblock/core/common/flatbuffer_data.h"
#include "components/adblock/core/resources/grit/adblock_resources.h"
#include "components/adblock/core/schema/filter_list_schema_generated.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/testing_pref_service.h"
#include "crypto/sha2.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {

class AdblockSubscriptionValidatorImplTest : public testing::Test {
 public:
  void SetUp() override {
    common::prefs::RegisterProfilePrefs(pref_service_.registry());
  }

  std::unique_ptr<SubscriptionValidatorImpl> MakeValidator(
      const std::string& current_schema_version) {
    return std::make_unique<SubscriptionValidatorImpl>(&pref_service_,
                                                       current_schema_version);
  }

  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple pref_service_;
};

TEST_F(AdblockSubscriptionValidatorImplTest, EmptyPrefMeansNoSignature) {
  auto validator = MakeValidator(CurrentSchemaVersion());
  auto buffer = utils::MakeFlatbufferDataFromResourceBundle(
      IDR_ADBLOCK_FLATBUFFER_EASYLIST);
  base::FilePath path(FILE_PATH_LITERAL("x.fb"));
  EXPECT_FALSE(validator->IsSignatureValid().Run(*buffer, path));
}

TEST_F(AdblockSubscriptionValidatorImplTest,
       StoredSignatureVisibleOnlyAfterRecreatingCallback) {
  auto validator = MakeValidator(CurrentSchemaVersion());
  auto initial_callback = validator->IsSignatureValid();
  // Store a new signature after the initial callback was created.
  auto buffer = utils::MakeFlatbufferDataFromResourceBundle(
      IDR_ADBLOCK_FLATBUFFER_EASYLIST);
  base::FilePath path(FILE_PATH_LITERAL("x.fb"));
  validator->StoreTrustedSignature().Run(*buffer, path);
  task_environment_.RunUntilIdle();
  // |initial_callback| uses the initial state of prefs, not the
  // current state, to avoid race conditions. It will still return the old
  // result.
  EXPECT_FALSE(initial_callback.Run(*buffer, path));
  // Recreate the callback.
  auto new_callback = validator->IsSignatureValid();
  // The new state has the signature stored in previous life.
  EXPECT_TRUE(new_callback.Run(*buffer, path));
  // Only the file component of the path is the key, this allows moving
  // to a different storage directory if needed.
  base::FilePath reparented_path(
      base::FilePath(FILE_PATH_LITERAL("parent")).AppendASCII("x.fb"));
  EXPECT_TRUE(new_callback.Run(*buffer, reparented_path));
}

TEST_F(AdblockSubscriptionValidatorImplTest, StoreAndRemoveSignature) {
  auto validator = MakeValidator(CurrentSchemaVersion());
  auto buffer = utils::MakeFlatbufferDataFromResourceBundle(
      IDR_ADBLOCK_FLATBUFFER_EASYLIST);
  base::FilePath path(FILE_PATH_LITERAL("x.fb"));
  validator->StoreTrustedSignature().Run(*buffer, path);
  validator->RemoveStoredSignature().Run(path);
  task_environment_.RunUntilIdle();
  // Recreate the validator.
  validator = MakeValidator(CurrentSchemaVersion());
  // The signature was removed so it's no longer returned.
  EXPECT_FALSE(validator->IsSignatureValid().Run(*buffer, path));
}

TEST_F(AdblockSubscriptionValidatorImplTest,
       SchemaVersionChangeInvalidatesSignatures) {
  auto validator = MakeValidator(CurrentSchemaVersion());

  // Store a valid signature.
  auto buffer = utils::MakeFlatbufferDataFromResourceBundle(
      IDR_ADBLOCK_FLATBUFFER_EASYLIST);
  base::FilePath path(FILE_PATH_LITERAL("x.fb"));
  validator->StoreTrustedSignature().Run(*buffer, path);
  task_environment_.RunUntilIdle();

  // Simulate a schema version change by storing a kLastUsedSchemaVersion
  // different than the current one.
  pref_service_.SetString(common::prefs::kLastUsedSchemaVersion, "000");

  // Recreate the validator.
  validator = MakeValidator(CurrentSchemaVersion());
  // The signature is invalid because we're not allowed to read flatbuffers
  // created with a different schema version.
  EXPECT_FALSE(validator->IsSignatureValid().Run(*buffer, path));
}

TEST_F(AdblockSubscriptionValidatorImplTest, SignatureStoredViaKey) {
  auto validator = MakeValidator(CurrentSchemaVersion());

  // Store a valid signature.
  auto buffer = utils::MakeFlatbufferDataFromResourceBundle(
      IDR_ADBLOCK_FLATBUFFER_EASYLIST);
  base::FilePath path(FILE_PATH_LITERAL("x.fb"));
  validator->StoreTrustedSignature().Run(*buffer, path);
  task_environment_.RunUntilIdle();

  // When storing dictionary keys with dots in them, there's a difference if
  // you use "SetIntKey" vs "SetIntPath".
  // "Path" interprets dict["x.fb"]="hash" as {"x":{"fb":"hash"}} - the dot
  // indicates a deeper level of dict
  // "Key" interprets dict["x.fb"]="hash" as {"x.fb":"hash"} - the dot is part
  // of the key name
  const base::Value::Dict& pref =
      pref_service_.GetDict(common::prefs::kSubscriptionSignatures);
  ASSERT_TRUE(pref.FindString("x.fb"));
  EXPECT_EQ(*pref.FindString("x.fb"),
            base::Base64Encode(crypto::SHA256Hash((buffer->span()))));
}

TEST_F(AdblockSubscriptionValidatorImplTest,
       DifferentBufferFailsSignatureValidation) {
  auto validator = MakeValidator(CurrentSchemaVersion());

  // Store signature for IDR_ADBLOCK_FLATBUFFER_EASYLIST subscription.
  auto buffer = utils::MakeFlatbufferDataFromResourceBundle(
      IDR_ADBLOCK_FLATBUFFER_EASYLIST);
  base::FilePath path(FILE_PATH_LITERAL("x.fb"));
  validator->StoreTrustedSignature().Run(*buffer, path);
  task_environment_.RunUntilIdle();
  // Recreate the validator.
  validator = MakeValidator(CurrentSchemaVersion());
  // If a different buffer resides on the same path, the signature does not
  // match.
  auto different_buffer = utils::MakeFlatbufferDataFromResourceBundle(
      IDR_ADBLOCK_FLATBUFFER_EXCEPTIONRULES);
  EXPECT_FALSE(validator->IsSignatureValid().Run(*different_buffer, path));
}

TEST_F(AdblockSubscriptionValidatorImplTest,
       NewSchemaVersionInvalidatesSubscriptions) {
  auto validator = MakeValidator(CurrentSchemaVersion());

  // Store signature for IDR_ADBLOCK_FLATBUFFER_EASYLIST subscription.
  auto buffer = utils::MakeFlatbufferDataFromResourceBundle(
      IDR_ADBLOCK_FLATBUFFER_EASYLIST);
  base::FilePath path(FILE_PATH_LITERAL("x.fb"));
  validator->StoreTrustedSignature().Run(*buffer, path);
  task_environment_.RunUntilIdle();

  // Recreate the validator with new schema version.
  validator = MakeValidator(CurrentSchemaVersion() + std::string("new"));
  // Same buffer, same path, but it's no longer valid because schema has
  // changed.
  EXPECT_FALSE(validator->IsSignatureValid().Run(*buffer, path));
}

TEST_F(AdblockSubscriptionValidatorImplTest,
       IsSignatureValidCallbackAfterValidatorDestroyed) {
  auto validator = MakeValidator(CurrentSchemaVersion());

  // Store a valid signature.
  auto buffer = utils::MakeFlatbufferDataFromResourceBundle(
      IDR_ADBLOCK_FLATBUFFER_EASYLIST);
  const base::FilePath path(FILE_PATH_LITERAL("x.fb"));
  validator->StoreTrustedSignature().Run(*buffer, path);
  task_environment_.RunUntilIdle();

  auto is_signature_valid = validator->IsSignatureValid();

  // Validator dies now:
  validator.reset();

  // |is_signature_valid| still works, using the remembered initial state.
  const auto bad_path = base::FilePath().AppendASCII("bad_path.fb");
  EXPECT_TRUE(is_signature_valid.Run(*buffer, path));
  EXPECT_FALSE(is_signature_valid.Run(*buffer, bad_path));
}

TEST_F(AdblockSubscriptionValidatorImplTest,
       StoreSignatureCallbackAfterValidatorDestroyed) {
  auto validator = MakeValidator(CurrentSchemaVersion());

  // Retrieve callback.
  auto store_signature = validator->StoreTrustedSignature();

  // Validator dies now:
  validator.reset();

  // Store a valid signature. This does nothing, since validator is dead.
  const auto buffer = utils::MakeFlatbufferDataFromResourceBundle(
      IDR_ADBLOCK_FLATBUFFER_EASYLIST);
  const base::FilePath path(FILE_PATH_LITERAL("x.fb"));
  std::move(store_signature).Run(*buffer, path);
  task_environment_.RunUntilIdle();

  // Verify that storing a signature did not have effects on future validators.
  validator = MakeValidator(CurrentSchemaVersion());
  // The signature wasn't stored and it is not recognized.
  EXPECT_FALSE(validator->IsSignatureValid().Run(*buffer, path));
}

TEST_F(AdblockSubscriptionValidatorImplTest,
       RemoveSignatureCallbackAfterValidatorDestroyed) {
  auto validator = MakeValidator(CurrentSchemaVersion());

  // Store a valid signature. This does nothing, since validator is dead.
  const auto buffer = utils::MakeFlatbufferDataFromResourceBundle(
      IDR_ADBLOCK_FLATBUFFER_EASYLIST);
  const base::FilePath path(FILE_PATH_LITERAL("x.fb"));
  validator->StoreTrustedSignature().Run(*buffer, path);
  task_environment_.RunUntilIdle();

  auto remove_signature = validator->RemoveStoredSignature();

  // Validator dies now:
  validator.reset();

  // Try removing signature after the validator has died, this does nothing.
  std::move(remove_signature).Run(path);

  // Verify that removing a signature did not have effects on future validators.
  validator = MakeValidator(CurrentSchemaVersion());
  // The signature wasn't removed and it is still recognized.
  EXPECT_TRUE(validator->IsSignatureValid().Run(*buffer, path));
}

}  // namespace adblock
