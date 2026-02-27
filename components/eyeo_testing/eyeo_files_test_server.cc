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

#include "base/at_exit.h"
#include "base/check.h"
#include "base/command_line.h"
#include "base/containers/span.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/logging/logging_settings.h"
#include "base/no_destructor.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/test/scoped_run_loop_timeout.h"
#include "base/test/task_environment.h"
#include "base/test/test_switches.h"
#include "base/test/test_timeouts.h"
#include "crypto/obsolete/md5.h"
#include "net/http/http_request_headers.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_response.h"

base::FilePath& GetFilesPath() {
  static base::NoDestructor<base::FilePath> g_files_path;
  return *g_files_path;
}

std::unique_ptr<net::test_server::HttpResponse> RequestHandler(
    const net::test_server::HttpRequest& request) {
  auto method = request.method;
  if (method != net::test_server::HttpMethod::METHOD_GET &&
      method != net::test_server::HttpMethod::METHOD_HEAD) {
    return nullptr;
  }
  bool is_head = (method == net::test_server::HttpMethod::METHOD_HEAD);
  LOG(INFO) << "Received " << (is_head ? "HEAD" : "GET")
            << " request for url: " << request.GetURL().spec();
  VLOG(1) << "Received request with headers: " << request.all_headers;
  auto file_name = request.GetURL().ExtractFileName();
  if (file_name.empty()) {
    return nullptr;
  }
  auto response = std::make_unique<net::test_server::BasicHttpResponse>();
  base::FilePath file_path = GetFilesPath().AppendASCII(file_name.c_str());
  if (!base::PathExists(file_path)) {
    LOG(WARNING) << "Cannot find content for: " << file_path;
    response->set_code(net::HTTP_NOT_FOUND);
    return response;
  }
  std::string file_content;
  DCHECK(base::ReadFileToString(file_path, &file_content));
  if (base::StartsWith(request.relative_url, "/snippets/isolated-first-all")) {
    const auto request_etag =
        request.headers.find(net::HttpRequestHeaders::kIfNoneMatch);
    auto hash =
        crypto::obsolete::Md5::HashForTesting(base::as_byte_span(file_content));
    const auto server_etag = base::ToLowerASCII(base::HexEncode(hash));
    if (is_head) {
      response->set_code(net::HTTP_OK);
    } else if ((request_etag != request.headers.end()) &&
               (server_etag == request_etag->second)) {
      response->set_code(net::HTTP_NOT_MODIFIED);
    } else {
      response->set_code(net::HTTP_OK);
      response->set_content_type("text/plain");
      response->set_content(file_content);
    }
    response->AddCustomHeader("ETag", server_etag);
  } else if (base::StartsWith(request.relative_url, "/recommendations.json")) {
    response->set_code(net::HTTP_OK);
    response->set_content_type("application/json");
    response->set_content(file_content);
  } else {
    response->set_code(net::HTTP_OK);
    response->set_content_type("text/plain");
    std::string list_content =
        base::StringPrintf(R"([Adblock Plus 2.0]\n\n%s)", file_content.c_str());
    response->set_content(list_content);
  }
  return response;
}

void InitializeGlobals(int argc, char* argv[]) {
  base::CommandLine::Init(argc, argv);
  logging::LoggingSettings logging_settings;
  logging_settings.logging_dest = logging::LOG_TO_STDERR;
  logging::InitLogging(logging_settings);
  TestTimeouts::Initialize();
}

int main(int argc, char* argv[]) {
  base::AtExitManager at_exit_manager;
  InitializeGlobals(argc, argv);
  if (base::CommandLine::ForCurrentProcess()->HasSwitch("help")) {
    LOG(INFO)
        << "\n\nInstantiates a server to serve files requested from the domain "
           "easylist-downloads.adblockplus.org: snippets library "
           "(isolated-first-all.jst), filter lists and recommendations.json. "
           "\n\nFiles to serve (like easylist.txt) are searched in current "
           "directory unless --dir argument is provided. When requested file "
           "is missing then 404 is returned."
           "\n\nAllows to specify a custom port by --port argument.";
    return 0;
  }
  const std::string directory =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII("dir");
  if (directory.empty()) {
    base::GetCurrentDirectory(&GetFilesPath());
  } else {
    GetFilesPath() = base::FilePath::FromASCII(directory.c_str());
  }
  if (!base::PathExists(GetFilesPath())) {
    LOG(ERROR) << "Directory to serve files from does not exist: "
               << GetFilesPath();
    return 1;
  }
  int port = 0;
  const std::string port_str =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII("port");
  if (!base::StringToInt(port_str, &port)) {
    port = 0;
  }
  base::test::TaskEnvironment task_environment;
  net::EmbeddedTestServer test_server(net::EmbeddedTestServer::TYPE_HTTPS);
  CHECK(test_server.InitializeAndListen(port));
  const std::string server_host_and_port =
      test_server.host_port_pair().ToString();
  LOG(INFO) << "Eyeo snippets update test server running on "
            << server_host_and_port << ", serves files from " << GetFilesPath();
  LOG(INFO)
      << "Start the browser with the following command line arguments:\n"
      << R"(--host-resolver-rules="MAP easylist-downloads.adblockplus.org:443 )"
      << server_host_and_port
      << R"(,EXCLUDE localhost" --ignore-certificate-errors --vmodule=*snippet*=2 --enable-logging=stderr)";
  test_server.RegisterDefaultHandler(base::BindRepeating(&RequestHandler));
  test_server.StartAcceptingConnections();

  base::test::ScopedDisableRunLoopTimeout disable_timeout;
  base::RunLoop().Run();
  return 0;
}
