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

#include "absl/types/optional.h"
#include "base/at_exit.h"
#include "base/check.h"
#include "base/command_line.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/test/scoped_run_loop_timeout.h"
#include "base/test/task_environment.h"
#include "base/test/test_switches.h"
#include "base/test/test_timeouts.h"
#include "net/http/http_request_headers.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_response.h"

// Will increase as new requests arrive
int g_next_token = 10000;

absl::optional<std::string> PrettyPrintedJson(std::string_view json) {
  const auto parsed = base::JSONReader::Read(json);
  if (!parsed) {
    LOG(INFO) << "Invalid json content: " << json;
    return absl::nullopt;
  }
  std::string pretty_printed;
  if (!base::JSONWriter::WriteWithOptions(
          *parsed, base::JSONWriter::OPTIONS_PRETTY_PRINT, &pretty_printed)) {
    return absl::nullopt;
  }
  return pretty_printed;
}

std::unique_ptr<net::test_server::HttpResponse> RequestHandler(
    const net::test_server::HttpRequest& request) {
  VLOG(1) << "Received request with headers " << request.all_headers;

  const auto host_header = request.headers.find(net::HttpRequestHeaders::kHost);
  if (host_header == request.headers.end() ||
      !base::EndsWith(host_header->second, "telemetry.eyeo.com")) {
    VLOG(1) << "This request is not bound for the eyeometry service, ignoring";
    auto response = std::make_unique<net::test_server::BasicHttpResponse>();
    response->set_content(
        "This navigation was intercepted by eyeometry_test_server but this "
        "does not look like a valid eyeometry ping");
    response->set_code(net::HTTP_OK);
    // TODO(mpawlowski): re-send the request to real Internet?
    return response;
  }

  const auto auth_bearer_token =
      request.headers.find(net::HttpRequestHeaders::kAuthorization);

  const auto content = PrettyPrintedJson(request.content);

  LOG(INFO) << "Received telemetry request for:\n"
            << "   host: " << host_header->second << "\n"
            << "   path: " << request.relative_url << "\n"
            << "   authorization header: "
            << (auth_bearer_token != request.headers.end()
                    ? auth_bearer_token->second
                    : "NONE, missing eyeo_telemetry_activeping_auth_token?")
            << "\n"
            << "    content: "
            << (content ? *content : "NONE, missing content?");

  auto response = std::make_unique<net::test_server::BasicHttpResponse>();
  const auto response_content = base::ReplaceStringPlaceholders(
      R"({"token": "$1"})", {base::NumberToString(g_next_token)}, nullptr);
  g_next_token += 10000;
  response->set_content(response_content);
  response->set_content_type("application/json");
  response->set_code(net::HTTP_OK);
  LOG(INFO) << "Sending response with content: " << response->content();
  LOG(INFO) << "Next token will be: " << g_next_token;
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
  LOG(INFO) << "Eyeometry test server running on " << server_host_and_port;
  LOG(INFO)
      << "Start the browser with the following command line arguments:\n"
      << R"(--host-resolver-rules="MAP *.telemetry.eyeo.com:443 )"
      << server_host_and_port
      << R"(,EXCLUDE localhost"  --ignore-certificate-errors --vmodule=*telemetry*=1,*activeping*=1)";
  test_server.RegisterDefaultHandler(base::BindRepeating(&RequestHandler));
  test_server.StartAcceptingConnections();

  base::test::ScopedDisableRunLoopTimeout disable_timeout;
  base::RunLoop().Run();
  return 0;
}
