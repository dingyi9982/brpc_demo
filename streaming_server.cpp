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

#include "chasis.pb.h"
#include <brpc/server.h>
#include <brpc/stream.h>
#include <butil/logging.h>
#include <gflags/gflags.h>
#include <thread>

DEFINE_int32(port, 8002, "TCP Port of this server");
DEFINE_int32(idle_timeout_s, -1,
             "Connection will be closed if there is no "
             "read/write operations during the last `idle_timeout_s'");

class StreamReceiver : public brpc::StreamInputHandler {
public:
  virtual int on_received_messages(brpc::StreamId id,
                                   butil::IOBuf *const messages[],
                                   size_t size) override {
    std::ostringstream os;
    for (size_t i = 0; i < size; ++i) {
      os << "msg[" << i << "]=" << *messages[i];
    }
    LOG(INFO) << "Received from Stream=" << id << ": " << os.str();
    return 0;
  }
  virtual void on_idle_timeout(brpc::StreamId id) override {
    LOG(INFO) << "Stream=" << id << " has no data transmission for a while";
  }
  virtual void on_closed(brpc::StreamId id) override {
    LOG(INFO) << "Stream=" << id << " is closed";
  }
  bool IsClosed() { return is_closed; };

private:
  bool is_closed = false;
};

// Your implementation of armctl::ChasisControl
class StreamingService : public armctl::Chasis {
public:
  StreamingService() {}
  virtual ~StreamingService() {
    for (const auto &thread : threads_) {
      if (thread->joinable()) {
        thread->join();
      }
    }
  }
  virtual void StartGetPose(google::protobuf::RpcController *controller,
                            const google::protobuf::Empty * /*request*/,
                            google::protobuf::Empty * /*response*/,
                            google::protobuf::Closure *done) {
    // This object helps you to call done->Run() in RAII style. If you need
    // to process the request asynchronously, pass done_guard.release().
    brpc::ClosureGuard done_guard(done);

    brpc::Controller *cntl = static_cast<brpc::Controller *>(controller);
    brpc::StreamId sd;
    brpc::StreamOptions stream_options;
    std::shared_ptr<StreamReceiver> sr = std::make_shared<StreamReceiver>();
    stream_options.handler = sr.get();
    if (brpc::StreamAccept(&sd, *cntl, &stream_options) != 0) {
      cntl->SetFailed("Fail to accept stream");
      return;
    }

    threads_.emplace_back(new std::thread([cntl, sd, sr]() {
      while (!brpc::IsAskedToQuit() && !sr->IsClosed()) {
        butil::IOBuf msg;
        armctl::Pose pose;
        pose.set_x(1.0f);
        pose.set_y(2.0f);
        pose.set_theta(3.0f);
        butil::IOBufAsZeroCopyOutputStream wrapper(&msg);
        pose.SerializeToZeroCopyStream(&wrapper);
        brpc::StreamWrite(sd, msg);
        sleep(1);
      }
      brpc::StreamClose(sd);
    }));
  }

private:
  std::vector<std::unique_ptr<std::thread>> threads_;
};

int main(int argc, char *argv[]) {
  // Parse gflags. We recommend you to use gflags as well.
  GFLAGS_NS::ParseCommandLineFlags(&argc, &argv, true);

  // Generally you only need one Server.
  brpc::Server server;

  // Instance of your service.
  StreamingService chasis_service_impl;

  // Add the service into server. Notice the second parameter, because the
  // service is put on stack, we don't want server to delete it, otherwise
  // use brpc::SERVER_OWNS_SERVICE.
  if (server.AddService(&chasis_service_impl,
                        brpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
    LOG(ERROR) << "Fail to add service";
    return -1;
  }

  // Start the server.
  brpc::ServerOptions options;
  options.idle_timeout_sec = FLAGS_idle_timeout_s;
  if (server.Start(FLAGS_port, &options) != 0) {
    LOG(ERROR) << "Fail to start EchoServer";
    return -1;
  }

  // Wait until Ctrl-C is pressed, then Stop() and Join() the server.
  server.RunUntilAskedToQuit();
  return 0;
}
