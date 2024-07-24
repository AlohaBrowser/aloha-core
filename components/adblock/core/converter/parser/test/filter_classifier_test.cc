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

#include "components/adblock/core/converter/parser/filter_classifier.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {

TEST(AdblockFilterClassifierTest, ClassifyElemHideFilter) {
  EXPECT_EQ(FilterType::ElemHide, FilterClassifier::Classify("##selectors"));
}

TEST(AdblockFilterClassifierTest, ClassifyElemHideFilterWithRemove) {
  EXPECT_EQ(FilterType::Remove,
            FilterClassifier::Classify("##selectors{remove:true;}"));
  EXPECT_EQ(FilterType::Remove,
            FilterClassifier::Classify("##selectors   {  remove :  true;  }"));
}

TEST(AdblockFilterClassifierTest, ClassifyElemHideFilterWithInlineCss) {
  EXPECT_EQ(FilterType::InlineCss,
            FilterClassifier::Classify("##selectors{some_inline css}"));
  EXPECT_EQ(FilterType::InlineCss,
            FilterClassifier::Classify("##selectors   { some  in line _css}"));
}

TEST(AdblockFilterClassifierTest, ClassifyElemHideExceptionFilter) {
  EXPECT_EQ(FilterType::ElemHideException,
            FilterClassifier::Classify("#@#selectors"));
}

TEST(AdblockFilterClassifierTest, ClassifyElemHideEmulationFilter) {
  EXPECT_EQ(FilterType::ElemHideEmulation,
            FilterClassifier::Classify("#?#advanced-selectors()"));
}

TEST(AdblockFilterClassifierTest, ClassifyElemHideEmulationFilterWithRemove) {
  EXPECT_EQ(FilterType::Remove, FilterClassifier::Classify(
                                    "#?#advanced-selectors(){remove:true;}"));
  EXPECT_EQ(FilterType::Remove,
            FilterClassifier::Classify(
                "#?#advanced-selectors()   {  remove :  true;  }"));
}

TEST(AdblockFilterClassifierTest,
     ClassifyElemHideEmulationFilterWithInlineCss) {
  EXPECT_EQ(
      FilterType::InlineCss,
      FilterClassifier::Classify("#?#advanced-selectors(){some_inline css}"));
  EXPECT_EQ(FilterType::InlineCss,
            FilterClassifier::Classify(
                "#?#advanced-selectors(){ some  in line _css}"));
}

TEST(AdblockFilterClassifierTest, ClassifyEmptyInput) {
  EXPECT_EQ(FilterType::Url, FilterClassifier::Classify(""));
}

TEST(AdblockFilterClassifierTest, ClasifyFilterWithoutSeparator) {
  EXPECT_EQ(FilterType::Url,
            FilterClassifier::Classify("filter_wo_separator_is_url_filter"));
}

}  // namespace adblock
