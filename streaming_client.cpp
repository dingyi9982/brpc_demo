#include "chasis.pb.h"
#include <brpc/channel.h>
#include <brpc/stream.h>
#include <butil/logging.h>
#include <gflags/gflags.h>

DEFINE_string(server, "0.0.0.0:8002", "IP Address of server");
DEFINE_int32(timeout_ms, 1000, "RPC timeout in milliseconds");
DEFINE_int32(max_retry, 3, "Max retries(not including the first RPC)");

class StreamReceiver : public brpc::StreamInputHandler {
public:
  virtual int on_received_messages(brpc::StreamId id,
                                   butil::IOBuf *const messages[],
                                   size_t size) override {
    std::ostringstream os;
    for (size_t i = 0; i < size; ++i) {
      armctl::Pose pose;
      butil::IOBufAsZeroCopyInputStream wrapper(*messages[i]);
      pose.ParseFromZeroCopyStream(&wrapper);
      os << "msg[" << i << "]=Pose(" << pose.x() << "," << pose.y() << ","
         << pose.theta() << ")";
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
};

int main(int argc, char *argv[]) {
  // Parse gflags. We recommend you to use gflags as well.
  GFLAGS_NS::ParseCommandLineFlags(&argc, &argv, true);

  // A Channel represents a communication line to a Server. Notice that
  // Channel is thread-safe and can be shared by all threads in your program.
  brpc::Channel channel;

  // Initialize the channel, NULL means using default options.
  brpc::ChannelOptions options;
  options.timeout_ms = FLAGS_timeout_ms /*milliseconds*/;
  options.max_retry = FLAGS_max_retry;
  if (channel.Init(FLAGS_server.c_str(), &options) != 0) {
    LOG(ERROR) << "Fail to initialize channel";
    return -1;
  }

  // Normally, you should not call a Channel directly, but instead construct
  // a stub Service wrapping it. stub can be shared by all threads as well.
  armctl::Chasis_Stub stub(&channel);
  brpc::Controller cntl;
  brpc::StreamId stream;

  StreamReceiver receiver;
  brpc::StreamOptions stream_options;
  stream_options.handler = &receiver;

  if (brpc::StreamCreate(&stream, cntl, &stream_options) != 0) {
    LOG(ERROR) << "Fail to create stream";
    return -1;
  }
  LOG(INFO) << "Created Stream=" << stream;
  google::protobuf::Empty empty;
  stub.StartGetPose(&cntl, &empty, nullptr, nullptr);
  if (cntl.Failed()) {
    LOG(ERROR) << "Fail to connect stream, " << cntl.ErrorText();
    return -1;
  }

  while (!brpc::IsAskedToQuit()) {
    sleep(1);
  }

  CHECK_EQ(0, brpc::StreamClose(stream));
  return 0;
}
