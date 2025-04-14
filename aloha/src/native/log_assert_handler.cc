// Copyright 2024 Aloha Mobile Ltd.

// Permission is hereby granted, free of charge, to any person obtaining 
// a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation 
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the Software
// is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION 
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "log_assert_handler.h"

#include <memory>
#include <vector>
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/logging.h"
#include "base/debug/debugger.h"
#include "base/strings/string_split.h"

namespace aloha {
namespace {

// Lines for ignore should be cleared when updating chromium.
constexpr auto g_any_line = 0;
constexpr std::pair<const char*, int> g_ignore_files[] = {
  {"dummy invalid file", -1},
};

// Raw-pointer is workaround for 'error: declaration requires an exit-time destructor'
static auto* g_ignore_messages = new std::vector<std::string>{{
  "Check failed: false. must increase kMaxCategories",
  "Check failed: has_descendants_for_table_part_ == other.has_descendants_for_table_part_",
  "assert(surface->backendFormat() == fFormat)",
  "Check failed: !paint_image_.sk_image_->isLazyGenerated().",
  "Check failed: impl_throughput().frames_expected > impl_throughput().frames_produced",
  "Check failed: impl_throughput().frames_produced < impl_throughput().frames_expected",
  "Check failed: impl_throughput_.frames_produced <= impl_throughput_.frames_expected",
  "Check failed: latest_swap_times_.size() <=",
  "Check failed: features::NeedThreadSafeAndroidMedia() == !!lock_",
  "Check failed: g_null_atom == ComputeBaseComputedStyleDiff", // Accept cookies dialog in www.dailymotion.com
  "Check failed: !*GetBaseSyncPrimitivesDisallowedTls()",
  "Check failed: GLSurfaceEGL::GetGLDisplayEGL()->IsAndroidNativeFenceSyncSupported()", // Start emulator
  "Check failed: CanAdvanceTo(next_state)",
  "checker.CalledOnValidThread(&bound_at)", // Application exit
  "Check failed: CalledOnValidSequence()", // Application exit
  "Check failed: !did_submit_in_last_frame_",
  "Check failed: depends_on_percentage_block_size_ == other.depends_on_percentage_block_size_", // On facebook
  "Debug check failed: deferred_bytes <= marked_bytes_", // On vhmovies.net
  "Debug check failed: !i_isolate->is_execution_terminating()", // On vhmovies.net
  "Check failed: !entry || entry->initiator_origin() == state.initiator_origin", // first load of theflixer.tv
  "Check failed: deserialized_referrer.url == entry->GetReferrer().url", // first load of theflixer.tv
  "Check failed: deserialized_referrer.policy == entry->GetReferrer().policy", // first load of theflixer.tv
  "Check failed: location.ContainsPoint(gfx::PointF(point))", // hit-test on hdrezka.tv
  "Check failed: inline_size >= border_padding.InlineSum", // Open chip.de
  "Check failed: !block.IsTableCell()",
}};

// Raw-pointer is workaround for 'error: declaration requires an exit-time destructor'
logging::ScopedLogAssertHandler* g_assert_handler = nullptr;

bool IsIgnoredAssert(const char* assert_file, int assert_line, const std::string_view assert_message) {
  if (std::end(*g_ignore_messages) != std::find_if(
      std::begin(*g_ignore_messages), std::end(*g_ignore_messages),
      [&assert_message] (const auto& ignored_message) {
        return assert_message.find(ignored_message) != std::string_view::npos;
      })) {
    return true;
  }

  const auto* it = std::find_if(
      std::begin(g_ignore_files), std::end(g_ignore_files),
      [&assert_file] (const auto& ignored_pair) {
        return std::string_view{assert_file}.find(ignored_pair.first) != std::string_view::npos;
      });
  if (it != std::end(g_ignore_files) &&
      (it->second == g_any_line || it->second == assert_line)) {
    return true;
  }
  return false;
}

} // namespace

void SetLogAssertHandler() {
  DCHECK(g_assert_handler == nullptr);
  g_assert_handler = new logging::ScopedLogAssertHandler{
    base::BindRepeating(
      [] (const char* file, int line,
          const std::string_view message, const std::string_view stack_trace) {
        if (!IsIgnoredAssert(file, line, message)) {
          base::debug::BreakDebugger();
        }
      })};
}

void AddIgnoredDcheck(std::string ignore_substring) {
  g_ignore_messages->push_back(std::move(ignore_substring));
}

}  // namespace aloha
