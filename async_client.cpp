// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

// A client sending requests to server asynchronously every 1 second.

#include "echo.pb.h"
#include <brpc/channel.h>
#include <butil/logging.h>
#include <butil/time.h>
#include <gflags/gflags.h>

DEFINE_bool(send_attachment, true, "Carry attachment along with requests");
DEFINE_string(server, "0.0.0.0:8002", "IP Address of server");
DEFINE_int32(timeout_ms, 100, "RPC timeout in milliseconds");
DEFINE_int32(max_retry, 3, "Max retries(not including the first RPC)");

void HandleEchoResponse(brpc::Controller *cntl,
                        brpc_demo::EchoResponse *response) {
  // std::unique_ptr makes sure cntl/response will be deleted before returning.
  std::unique_ptr<brpc::Controller> cntl_guard(cntl);
  std::unique_ptr<brpc_demo::EchoResponse> response_guard(response);

  if (cntl->Failed()) {
    LOG(WARNING) << "Fail to send EchoRequest, " << cntl->ErrorText();
    return;
  }
  LOG(INFO) << "Received response from " << cntl->remote_side() << ": "
            << response->message()
            << " (attached=" << cntl->response_attachment() << ")"
            << " latency=" << cntl->latency_us() << "us";
}

int main(int argc, char *argv[]) {
  // Parse gflags. We recommend you to use gflags as well.
  GFLAGS_NS::ParseCommandLineFlags(&argc, &argv, true);

  // A Channel represents a communication line to a Server. Notice that
  // Channel is thread-safe and can be shared by all threads in your program.
  brpc::Channel channel;

  // Initialize the channel, NULL means using default options.
  brpc::ChannelOptions options;
  //   options.protocol = FLAGS_protocol;
  //   options.connection_type = FLAGS_connection_type;
  options.timeout_ms = FLAGS_timeout_ms /*milliseconds*/;
  options.max_retry = FLAGS_max_retry;
  if (channel.Init(FLAGS_server.c_str(), &options) != 0) {
    LOG(ERROR) << "Fail to initialize channel";
    return -1;
  }

  // Normally, you should not call a Channel directly, but instead construct
  // a stub Service wrapping it. stub can be shared by all threads as well.
  brpc_demo::EchoService_Stub stub(&channel);

  // Send a request and wait for the response every 1 second.
  int log_id = 0;
  while (!brpc::IsAskedToQuit()) {
    // Since we are sending asynchronous RPC (`done' is not NULL),
    // these objects MUST remain valid until `done' is called.
    // As a result, we allocate these objects on heap
    brpc_demo::EchoResponse *response = new brpc_demo::EchoResponse();
    brpc::Controller *cntl = new brpc::Controller();

    // Notice that you don't have to new request, which can be modified
    // or destroyed just after stub.Echo is called.
    brpc_demo::EchoRequest request;
    request.set_message("hello world");

    cntl->set_log_id(log_id++); // set by user
    if (FLAGS_send_attachment) {
      // Set attachment which is wired to network directly instead of
      // being serialized into protobuf messages.
      cntl->request_attachment().append("foo");
    }

    // We use protobuf utility `NewCallback' to create a closure object
    // that will call our callback `HandleEchoResponse'. This closure
    // will automatically delete itself after being called once
    google::protobuf::Closure *done =
        brpc::NewCallback(&HandleEchoResponse, cntl, response);
    stub.Echo(cntl, &request, response, done);

    // This is an asynchronous RPC, so we can only fetch the result
    // inside the callback
    sleep(1);
  }

  LOG(INFO) << "EchoClient is going to quit";
  return 0;
}
