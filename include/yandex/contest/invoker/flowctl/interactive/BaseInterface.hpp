#pragma once

#include <boost/version.hpp>
#if BOOST_VERSION < 106100
#define BOOST_NO_CXX11_VARIADIC_TEMPLATES
#endif

#include <yandex/contest/invoker/flowctl/interactive/BufferedWrapper.hpp>

#include <boost/bind.hpp>
#include <boost/signals2.hpp>

namespace yandex {
namespace contest {
namespace invoker {
namespace flowctl {
namespace interactive {

namespace detail {
template <typename Parent>
class StrandInject : public Parent {
 public:
  template <typename... Args>
  StrandInject(boost::asio::io_service &ioService, Args &&... args)
      : Parent(std::forward<Args>(args)...), strand_(ioService) {}

 protected:
  boost::asio::io_service::strand strand_;
};
}  // namespace detail

class BaseInterface : public detail::StrandInject<BufferedWrapper> {
 public:
  using ErrorSignal = boost::signals2::signal<void(boost::system::error_code,
                                                   std::size_t size)>;
  using ErrorSlot = ErrorSignal::slot_type;
  using ErrorExtendedSlot = ErrorSignal::extended_slot_type;

 public:
  BaseInterface(UnderlyingSource &underlyingSource,
                UnderlyingSink &underlyingSink,
                const std::size_t readLimitBytes)
      : detail::StrandInject<BufferedWrapper>(
            underlyingSource.get_io_service(), underlyingSource, underlyingSink,
            strand_.wrap(
                boost::bind(&BaseInterface::handle_source_read, this,
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred)),
            strand_.wrap(
                boost::bind(&BaseInterface::handle_source_write, this,
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred)),
            strand_.wrap(
                boost::bind(&BaseInterface::handle_sink_read, this,
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred)),
            strand_.wrap(
                boost::bind(&BaseInterface::handle_sink_write, this,
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred))),
        readLimitBytes_(readLimitBytes) {}

  ErrorSignal sourceReadError;
  ErrorSignal sourceWriteError;
  ErrorSignal sinkReadError;
  ErrorSignal sinkWriteError;
  ErrorSignal readLimitExceeded;

 private:
  void handle_source_read(const boost::system::error_code &ec,
                          const std::size_t size) {
    if (ec && ec != boost::asio::error::eof) {
      sourceReadError(ec, size);
      return;
    }

    readBytes_ += size;
    if (readBytes_ > readLimitBytes_) {
      readLimitExceeded(ec, size);
      terminate();
    }
  }

  void handle_source_write(const boost::system::error_code &ec,
                           const std::size_t size) {
    if (ec) {
      sourceWriteError(ec, size);
      return;
    }
  }

  void handle_sink_read(const boost::system::error_code &ec,
                        const std::size_t size) {
    if (ec && ec != boost::asio::error::eof) {
      sinkReadError(ec, size);
      return;
    }
  }

  void handle_sink_write(const boost::system::error_code &ec,
                         const std::size_t size) {
    if (ec) {
      sinkWriteError(ec, size);
      return;
    }
  }

 private:
  const std::size_t readLimitBytes_;
  std::size_t readBytes_ = 0;
};

}  // namespace interactive
}  // namespace flowctl
}  // namespace invoker
}  // namespace contest
}  // namespace yandex
