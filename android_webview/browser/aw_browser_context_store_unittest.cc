// Copyright 2026 Aloha Mobile Ltd.
//
// ALOHA CU-86eqe7rd2 (U2): tests for the one-shot migration of public/private
// mode cookie files to the profile paths (bromium_migration).

#include "android_webview/browser/aw_browser_context_store.h"

#include <string>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace android_webview {

namespace {

base::FilePath Journal(const base::FilePath& db) {
  return base::FilePath(db.value() + FILE_PATH_LITERAL("-journal"));
}

void WriteFileOrDie(const base::FilePath& path, const std::string& content) {
  ASSERT_TRUE(base::WriteFile(path, content));
}

std::string ReadFileOrDie(const base::FilePath& path) {
  std::string content;
  EXPECT_TRUE(base::ReadFileToString(path, &content));
  return content;
}

class BromiumCookieMigrationTest : public testing::Test {
 protected:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    default_dir_ = temp_dir_.GetPath().Append(FILE_PATH_LITERAL("Default"));
    bromium_dir_ = default_dir_.Append(FILE_PATH_LITERAL("bromium"));
    ASSERT_TRUE(base::CreateDirectory(bromium_dir_));
    src_db_ = bromium_dir_.Append(FILE_PATH_LITERAL("Cookies.1"));
    dest_db_ = default_dir_.Append(FILE_PATH_LITERAL("Cookies"));
  }

  base::ScopedTempDir temp_dir_;
  base::FilePath default_dir_;
  base::FilePath bromium_dir_;
  base::FilePath src_db_;
  base::FilePath dest_db_;
};

// Happy path: db + journal are copied to an empty dest. The source is left in
// place — MigrateBromiumModeCookies wipes every old source afterwards via
// DeleteCookieStorePair.
TEST_F(BromiumCookieMigrationTest, MigratesDbWithJournal) {
  WriteFileOrDie(src_db_, "db-data");
  WriteFileOrDie(Journal(src_db_), "journal-data");

  EXPECT_TRUE(bromium_migration::MigrateCookieStorePair(src_db_, dest_db_));

  EXPECT_EQ(ReadFileOrDie(dest_db_), "db-data");
  EXPECT_EQ(ReadFileOrDie(Journal(dest_db_)), "journal-data");
  EXPECT_TRUE(base::PathExists(src_db_));
  EXPECT_TRUE(base::PathExists(Journal(src_db_)));
}

// No source → no-op (reported as success), existing dest untouched.
TEST_F(BromiumCookieMigrationTest, NoSourceIsNoOp) {
  WriteFileOrDie(dest_db_, "existing");
  WriteFileOrDie(Journal(dest_db_), "existing-journal");

  EXPECT_TRUE(bromium_migration::MigrateCookieStorePair(src_db_, dest_db_));

  EXPECT_EQ(ReadFileOrDie(dest_db_), "existing");
  EXPECT_EQ(ReadFileOrDie(Journal(dest_db_)), "existing-journal");
}

// Regression (public-session loss on upgrade, reproduced on-device 2026-08-14):
// the pre-profile builds keep a stale (empty) store at Default/Cookies, so an
// existing destination must be REPLACED by the source, not preserved.
// "Never re-copy over a live post-migration store" is now enforced one level
// up, by the marker file in MigrateBromiumModeCookies.
TEST_F(BromiumCookieMigrationTest, ReplacesStaleDestination) {
  WriteFileOrDie(src_db_, "real-cookies");
  WriteFileOrDie(Journal(src_db_), "real-journal");
  WriteFileOrDie(dest_db_, "stale-empty-store");
  WriteFileOrDie(Journal(dest_db_), "stale-journal");

  EXPECT_TRUE(bromium_migration::MigrateCookieStorePair(src_db_, dest_db_));

  EXPECT_EQ(ReadFileOrDie(dest_db_), "real-cookies");
  EXPECT_EQ(ReadFileOrDie(Journal(dest_db_)), "real-journal");
}

// Regression (alien hot journal): when the source has NO journal, a leftover
// destination journal must be deleted, not kept — SQLite pairs a journal with
// its db by file name alone and would replay the foreign journal into the
// copied db, corrupting it.
TEST_F(BromiumCookieMigrationTest, ClearsStaleDestJournalWhenSourceHasNone) {
  WriteFileOrDie(src_db_, "real-cookies");
  WriteFileOrDie(dest_db_, "stale-empty-store");
  WriteFileOrDie(Journal(dest_db_), "alien-hot-journal");

  EXPECT_TRUE(bromium_migration::MigrateCookieStorePair(src_db_, dest_db_));

  EXPECT_EQ(ReadFileOrDie(dest_db_), "real-cookies");
  EXPECT_FALSE(base::PathExists(Journal(dest_db_)));
}

// Crash after copy, before the marker/cleanup: the re-run re-copies the same
// source over its own previous copy — identical bytes, no data loss.
TEST_F(BromiumCookieMigrationTest, RerunRecopiesIdenticalSource) {
  WriteFileOrDie(src_db_, "db-data");
  WriteFileOrDie(Journal(src_db_), "journal-data");
  ASSERT_TRUE(bromium_migration::MigrateCookieStorePair(src_db_, dest_db_));

  EXPECT_TRUE(bromium_migration::MigrateCookieStorePair(src_db_, dest_db_));

  EXPECT_EQ(ReadFileOrDie(dest_db_), "db-data");
  EXPECT_EQ(ReadFileOrDie(Journal(dest_db_)), "journal-data");
}

// Regression (delete-dest-before-copy): a staging failure must leave the
// pre-existing destination pair fully intact — the session keeps running on
// it and the next start retries from unchanged inputs. Forced by occupying
// the temp path with a directory so the staged CopyFile fails.
TEST_F(BromiumCookieMigrationTest, StagingFailureLeavesDestIntact) {
  WriteFileOrDie(src_db_, "real-cookies");
  WriteFileOrDie(dest_db_, "stale-store");
  WriteFileOrDie(Journal(dest_db_), "stale-journal");
  ASSERT_TRUE(base::CreateDirectory(
      base::FilePath(dest_db_.value() + FILE_PATH_LITERAL(".migrating"))));

  EXPECT_FALSE(bromium_migration::MigrateCookieStorePair(src_db_, dest_db_));

  EXPECT_EQ(ReadFileOrDie(dest_db_), "stale-store");
  EXPECT_EQ(ReadFileOrDie(Journal(dest_db_)), "stale-journal");
  EXPECT_TRUE(base::PathExists(src_db_));
}

// Crash mid-copy (emulation: only a partial dest journal, no dest db, source
// intact) → the repeat completes the copy from the consistent source.
TEST_F(BromiumCookieMigrationTest, RerunAfterPartialCopyRecopies) {
  WriteFileOrDie(src_db_, "db-data");
  WriteFileOrDie(Journal(src_db_), "journal-data");
  WriteFileOrDie(Journal(dest_db_), "partial-journal");  // crash before db copy.

  EXPECT_TRUE(bromium_migration::MigrateCookieStorePair(src_db_, dest_db_));

  EXPECT_EQ(ReadFileOrDie(dest_db_), "db-data");
  EXPECT_EQ(ReadFileOrDie(Journal(dest_db_)), "journal-data");
}

// DeleteCookieStorePair removes both the db and its rollback journal.
TEST_F(BromiumCookieMigrationTest, DeleteCookieStorePairRemovesDbAndJournal) {
  WriteFileOrDie(src_db_, "db-data");
  WriteFileOrDie(Journal(src_db_), "journal-data");

  bromium_migration::DeleteCookieStorePair(src_db_);

  EXPECT_FALSE(base::PathExists(src_db_));
  EXPECT_FALSE(base::PathExists(Journal(src_db_)));
}

// Network state: regular files move, Cookies.* and subdirectories do not.
TEST_F(BromiumCookieMigrationTest, MovesNetworkStateSkipsCookiesAndDirs) {
  WriteFileOrDie(bromium_dir_.Append(FILE_PATH_LITERAL("Network Persistent State")),
                 "net-state");
  WriteFileOrDie(bromium_dir_.Append(FILE_PATH_LITERAL("TransportSecurity")),
                 "hsts");
  WriteFileOrDie(bromium_dir_.Append(FILE_PATH_LITERAL("Cookies.0")), "pub");
  WriteFileOrDie(bromium_dir_.Append(FILE_PATH_LITERAL("Cookies.0-journal")),
                 "pub-j");
  const base::FilePath downloads =
      bromium_dir_.Append(FILE_PATH_LITERAL("TemporaryDownloads"));
  ASSERT_TRUE(base::CreateDirectory(downloads));
  WriteFileOrDie(downloads.Append(FILE_PATH_LITERAL("video.mp4")), "video");

  bromium_migration::MoveBromiumNetworkStateFiles(bromium_dir_, default_dir_);

  EXPECT_EQ(ReadFileOrDie(default_dir_.Append(
                FILE_PATH_LITERAL("Network Persistent State"))),
            "net-state");
  EXPECT_EQ(ReadFileOrDie(default_dir_.Append(
                FILE_PATH_LITERAL("TransportSecurity"))),
            "hsts");
  // Cookies.* stay in place (migrated by MigrateCookieStorePair).
  EXPECT_TRUE(
      base::PathExists(bromium_dir_.Append(FILE_PATH_LITERAL("Cookies.0"))));
  EXPECT_TRUE(base::PathExists(
      bromium_dir_.Append(FILE_PATH_LITERAL("Cookies.0-journal"))));
  // TemporaryDownloads untouched.
  EXPECT_EQ(ReadFileOrDie(downloads.Append(FILE_PATH_LITERAL("video.mp4"))),
            "video");
}

// An occupied network-state destination is NOT overwritten (live state wins);
// the stale source is dropped instead.
TEST_F(BromiumCookieMigrationTest, NetworkStateDoesNotOverwriteExistingDest) {
  const base::FilePath src =
      bromium_dir_.Append(FILE_PATH_LITERAL("TransportSecurity"));
  const base::FilePath dest =
      default_dir_.Append(FILE_PATH_LITERAL("TransportSecurity"));
  WriteFileOrDie(src, "stale");
  WriteFileOrDie(dest, "live");

  bromium_migration::MoveBromiumNetworkStateFiles(bromium_dir_, default_dir_);

  EXPECT_EQ(ReadFileOrDie(dest), "live");
  EXPECT_FALSE(base::PathExists(src));
}

}  // namespace

}  // namespace android_webview
