// Copyright 2019-2020:
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

#ifndef UDPMAVLink20190815H
#define UDPMAVLink20190815H

#include "goby/middleware/io/mavlink/common.h"

#include "goby/middleware/io/udp_point_to_point.h"

namespace goby
{
namespace middleware
{
namespace io
{
template <const goby::middleware::Group& line_in_group,
          const goby::middleware::Group& line_out_group,
          PubSubLayer publish_layer = PubSubLayer::INTERPROCESS,
          PubSubLayer subscribe_layer = PubSubLayer::INTERTHREAD>
using UDPThreadMAVLinkBase = IOThreadMAVLink<
    line_in_group, line_out_group, publish_layer, subscribe_layer,
    UDPPointToPointThread<line_in_group, line_out_group, publish_layer, subscribe_layer>,
    goby::middleware::protobuf::UDPPointToPointConfig>;

/// \brief Reads/Writes MAVLink message packages from/to udp socket
/// \tparam line_in_group goby::middleware::Group to publish to after receiving data from the udp socket
/// \tparam line_out_group goby::middleware::Group to subcribe to for data to send to the udp socket
template <const goby::middleware::Group& line_in_group,
          const goby::middleware::Group& line_out_group,
          PubSubLayer publish_layer = PubSubLayer::INTERPROCESS,
          PubSubLayer subscribe_layer = PubSubLayer::INTERTHREAD>
class UDPThreadMAVLink
    : public UDPThreadMAVLinkBase<line_in_group, line_out_group, publish_layer, subscribe_layer>
{
  public:
    UDPThreadMAVLink(const goby::middleware::protobuf::UDPPointToPointConfig& config)
        : UDPThreadMAVLinkBase<line_in_group, line_out_group, publish_layer, subscribe_layer>(
              config)
    {
    }

    ~UDPThreadMAVLink() {}

  private:
    void async_read() override;

  private:
    boost::asio::ip::udp::endpoint sender_endpoint_;
};
} // namespace io
} // namespace middleware
} // namespace goby

template <const goby::middleware::Group& line_in_group,
          const goby::middleware::Group& line_out_group,
          goby::middleware::io::PubSubLayer publish_layer,
          goby::middleware::io::PubSubLayer subscribe_layer>
void goby::middleware::io::UDPThreadMAVLink<line_in_group, line_out_group, publish_layer,
                                            subscribe_layer>::async_read()
{
    this->mutable_socket().async_receive_from(
        boost::asio::buffer(this->buffer()), sender_endpoint_,
        [this](const boost::system::error_code& ec, std::size_t bytes_transferred) {
            if (!ec && bytes_transferred > 0)
            {
                this->try_parse(bytes_transferred);
                this->async_read();
            }
            else
            {
                this->handle_read_error(ec);
            }
        });
}

#endif
