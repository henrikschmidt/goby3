// Copyright 2020:
//   GobySoft, LLC (2013-)
//   Community contributors (see AUTHORS file)
// File authors:
//   Toby Schneider <toby@gobysoft.org>
//
//
// This file is part of the Goby Underwater Autonomy Project Libraries
// ("The Goby Libraries").
//
// The Goby Libraries are free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 2.1 of the License, or
// (at your option) any later version.
//
// The Goby Libraries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <boost/asio/ip/tcp.hpp>

#include "goby/middleware/io/detail/io_interface.h"
#include "goby/middleware/protobuf/tcp_config.pb.h"

namespace goby
{
namespace middleware
{
namespace io
{
namespace detail
{
template <const goby::middleware::Group& line_in_group,
          const goby::middleware::Group& line_out_group,
          // by default publish all incoming traffic to interprocess for logging
          PubSubLayer publish_layer = PubSubLayer::INTERPROCESS,
          // but only subscribe on interthread for outgoing traffic
          PubSubLayer subscribe_layer = PubSubLayer::INTERTHREAD,
          typename Config = goby::middleware::protobuf::TCPClientConfig>
class TCPClientThread : public IOThread<line_in_group, line_out_group, publish_layer,
                                        subscribe_layer, Config, boost::asio::ip::tcp::socket>
{
    using Base = IOThread<line_in_group, line_out_group, publish_layer, subscribe_layer, Config,
                          boost::asio::ip::tcp::socket>;

    using ConfigType = Config;

  public:
    /// \brief Constructs the thread.
    /// \param config A reference to the Protocol Buffers config read by the main application at launch
    TCPClientThread(const Config& config, int index = -1)
        : Base(config, index,
               std::string("tcp: ") + config.remote_address() + ":" +
                   std::to_string(config.remote_port()))
    {
        boost::asio::ip::tcp::resolver resolver(this->mutable_io());
        remote_endpoint_ =
            *resolver.resolve({boost::asio::ip::tcp::v4(), this->cfg().remote_address(),
                               std::to_string(this->cfg().remote_port())});
    }

    ~TCPClientThread() {}

  protected:
    void insert_endpoints(std::shared_ptr<goby::middleware::protobuf::IOData>& io_msg)
    {
        *io_msg->mutable_tcp_src() = endpoint_convert<protobuf::TCPEndPoint>(remote_endpoint_);
        *io_msg->mutable_tcp_dest() = endpoint_convert<protobuf::TCPEndPoint>(local_endpoint_);
    }

  private:
    /// \brief Starts an asynchronous write from data published
    void async_write(const std::string& bytes) override
    {
        boost::asio::async_write(
            this->mutable_socket(), boost::asio::buffer(bytes),
            [this](const boost::system::error_code& ec, std::size_t bytes_transferred) {
                if (!ec && bytes_transferred > 0)
                {
                    this->handle_write_success(bytes_transferred);
                }
                else
                {
                    this->handle_write_error(ec);
                }
            });
    }

    /// \brief Tries to open the tcp client socket, and if fails publishes an error
    void open_socket()
    {
        this->mutable_socket().connect(remote_endpoint_);
        local_endpoint_ = this->mutable_socket().local_endpoint();
    }

  private:
    boost::asio::ip::tcp::endpoint remote_endpoint_;
    boost::asio::ip::tcp::endpoint local_endpoint_;
};

} // namespace detail
} // namespace io
} // namespace middleware
} // namespace goby
