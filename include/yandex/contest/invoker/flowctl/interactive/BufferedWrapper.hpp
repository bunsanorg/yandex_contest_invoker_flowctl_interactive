#pragma once

#include <yandex/contest/system/unistd/Pipe.hpp>

#include <bunsan/asio/buffer_connection.hpp>

#include <boost/asio.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>

namespace yandex {
namespace contest {
namespace invoker {
namespace flowctl {
namespace interactive {

class BufferedWrapper : private boost::noncopyable {
 public:
  using Source = boost::asio::posix::stream_descriptor;
  using UnderlyingSource = boost::asio::posix::stream_descriptor;
  using Sink = boost::asio::posix::stream_descriptor;
  using UnderlyingSink = boost::asio::posix::stream_descriptor;
  using SourceBuffer = bunsan::asio::buffer_connection<UnderlyingSource, Sink>;
  using SinkBuffer = bunsan::asio::buffer_connection<Source, UnderlyingSink>;

  /// \note SinkBuffer::handler is the same
  using Handler = SourceBuffer::handler;

 public:
  BufferedWrapper(boost::asio::io_service &ioService,
                  UnderlyingSource &underlyingSource,
                  UnderlyingSink &underlyingSink,
                  const Handler &sourceReadHandler,
                  const Handler &sourceWriteHandler,
                  const Handler &sinkReadHandler,
                  const Handler &sinkWriteHandler)
      : underlyingSource_(underlyingSource),
        underlyingSink_(underlyingSink),
        sourceInput_(ioService, sourcePipe_.releaseWriteEnd().release()),
        sinkOutput_(ioService, sinkPipe_.releaseReadEnd().release()),
        source_(ioService, sourcePipe_.releaseReadEnd().release()),
        sink_(ioService, sinkPipe_.releaseWriteEnd().release()),
        sourceBuffer_(underlyingSource_, sourceInput_, sourceReadHandler,
                      sourceWriteHandler),
        sinkBuffer_(sinkOutput_, underlyingSink_, sinkReadHandler,
                    sinkWriteHandler) {}

  BufferedWrapper(UnderlyingSource &underlyingSource,
                  UnderlyingSink &underlyingSink,
                  const Handler &sourceReadHandler,
                  const Handler &sourceWriteHandler,
                  const Handler &sinkReadHandler,
                  const Handler &sinkWriteHandler)
      : BufferedWrapper(underlyingSource.get_io_service(), underlyingSource,
                        underlyingSink, sourceReadHandler, sourceWriteHandler,
                        sinkReadHandler, sinkWriteHandler) {}

  void start() {
    sourceBuffer_.start();
    sinkBuffer_.start();
  }

  Source &source() { return source_; }
  Sink &sink() { return sink_; }

  void close() {
    sourceBuffer_.close();
    sinkBuffer_.close();
  }

  void terminate() {
    sourceBuffer_.terminate();
    sinkBuffer_.terminate();
  }

 private:
  UnderlyingSource &underlyingSource_;
  UnderlyingSink &underlyingSink_;

  yandex::contest::system::unistd::Pipe sourcePipe_;
  yandex::contest::system::unistd::Pipe sinkPipe_;

  Sink sourceInput_;
  Source sinkOutput_;

  Source source_;
  Sink sink_;

  SourceBuffer sourceBuffer_;
  SinkBuffer sinkBuffer_;
};

}  // namespace interactive
}  // namespace flowctl
}  // namespace invoker
}  // namespace contest
}  // namespace yandex
