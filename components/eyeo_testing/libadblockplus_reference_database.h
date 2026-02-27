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

#ifndef COMPONENTS_EYEO_TESTING_LIBADBLOCKPLUS_REFERENCE_DATABASE_H_
#define COMPONENTS_EYEO_TESTING_LIBADBLOCKPLUS_REFERENCE_DATABASE_H_

#include <string>

#include "base/files/file_path.h"
#include "base/functional/callback_forward.h"
#include "components/eyeo_testing/test_request_reader.h"
#include "sql/database.h"
#include "url/gurl.h"

namespace adblock {
namespace test {

class LibadblockplusReferenceDatabase {
 public:
  struct ElemhideResults {
    ElemhideResults();
    ElemhideResults(std::string elemhide_css,
                    std::vector<std::string> elemhide_emu_selectors,
                    std::string snippets);
    ElemhideResults(const ElemhideResults&);
    ElemhideResults& operator=(const ElemhideResults&);
    ~ElemhideResults();
    std::string elemhide_css;
    std::vector<std::string> elemhide_emu_selectors;
    std::string snippets;
  };
  // Callback that returns if an underlying ad-blocking implementation would
  // block a request with given parameters.
  using FilterMatchCallback =
      base::RepeatingCallback<bool(const Request& request)>;
  // Callback that returns what element hiding content an underlying ad-blocking
  // implementation returns for a given request. Only called for requests with
  // content type SUBRESOURCE, as it would happen IRL.
  using ElemhideCallback =
      base::RepeatingCallback<ElemhideResults(const Request& request)>;

  enum class MismatchType {
    UrlFilterMatch,
    ElemhideCss,
    ElemhideEmuSelectors,
    Snippets
  };
  // Called to report a mismatch between stored reference result and tested
  // implementation's result.
  using MismatchCallback =
      base::RepeatingCallback<void(int line_number,
                                   MismatchType mismatch_type)>;

  explicit LibadblockplusReferenceDatabase(const base::FilePath& db_path);
  void StoreInputsFromFile(const base::FilePath& request_data_file,
                           FilterMatchCallback reference_match_implementation,
                           ElemhideCallback reference_elemhide_implementation);

  void CompareImplementationAgainstReference(
      FilterMatchCallback tested_match_implementation,
      ElemhideCallback tested_elemhide_implementation,
      MismatchCallback mismatch_callback,
      int task_id,
      int task_count);

  void CompareImplementationAgainstSpecificRequests(
      FilterMatchCallback tested_match_implementation,
      ElemhideCallback tested_elemhide_implementation,
      MismatchCallback mismatch_callback,
      const std::string& request_ids);

 protected:
  void CompareWithStatement(sql::Statement& select_request,
                            FilterMatchCallback tested_match_implementation,
                            ElemhideCallback tested_elemhide_implementation,
                            MismatchCallback mismatch_callback);
  void RecreateDatabase();
  sql::Database db_;
};
}  // namespace test
}  // namespace adblock

#endif  // COMPONENTS_EYEO_TESTING_LIBADBLOCKPLUS_REFERENCE_DATABASE_H_
