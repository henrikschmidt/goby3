// copyright 2009 t. schneider tes@mit.edu 
//
// this file is part of the Queue Library (libqueue),
// the goby-acomms message queue manager. goby-acomms is a collection of 
// libraries for acoustic underwater networking
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this software.  If not, see <http://www.gnu.org/licenses/>.


#ifndef QueueManager20091204_H
#define QueueManager20091204_H

#include <limits>
#include <set>
#include <boost/bind.hpp>
#include <boost/signal.hpp>

#include "goby/acomms/xml/xml_parser.h"
#include "goby/acomms/dccl.h"
#include "goby/acomms/protobuf/queue.pb.h"
#include "goby/acomms/protobuf/acomms_proto_helpers.h"

#include <map>
#include <deque>

#include "queue_exception.h"
#include "queue.h"


namespace goby
{
    namespace util
    {
        class FlexOstream;
    }
        
    
    namespace acomms
    {
        /// provides an API to the goby-acomms Queuing Library.
        class QueueManager
        {
          public:
            /// \name Constructors/Destructor
            //@{         
            /// \brief Default constructor.
            ///
            /// \param os std::ostream object or FlexOstream to capture all humanly readable runtime and debug information (optional).
            QueueManager(std::ostream* os = 0);

            /// Destructor.
            ~QueueManager() { }
        
            //@}
        
            /// \name Initialization Methods
            ///
            /// These methods are intended to be called before doing any work with the class.
            //@{


            void set_cfg(const protobuf::QueueManagerConfig& cfg);
            void merge_cfg(const protobuf::QueueManagerConfig& cfg);

            void add_flex_groups(util::FlexOstream* tout);
            
            
            //@}

            /// \name Application level Push/Receive Methods
            ///
            /// These methods are the primary higher level interface to the QueueManager. From here you can push messages
            /// and set the callbacks to use on received messages.
            //@{

            /// \brief Push a message using a QueueKey as a key.
            ///
            /// \param key QueueKey that references the %queue to push the message to.
            /// \param new_message ModemMessage to push.
            void push_message(const protobuf::ModemDataTransmission& new_message);        
            //@}
        
            /// \name Modem Driver level Push/Receive Methods
            ///
            /// These methods are the interface to the QueueManager from the %modem driver.
            //@{

            /// \brief Finds data to send to the %modem.
            /// 
            /// Data from the highest priority %queue(s) will be combined to form a message equal or less than the size requested in ModemMessage message_in. If using one of the classes inheriting ModemDriverBase, this method should be bound and passed to ModemDriverBase::set_datarequest_cb.
            /// \param message_in The ModemMessage containing the details of the request (source, destination, size, etc.)
            /// \param message_out The packed ModemMessage ready for sending by the modem. This will be populated by this function.
            /// \return true if successful in finding data to send, false if no data is available
            void handle_modem_data_request(const protobuf::ModemDataRequest& msg_request, protobuf::ModemDataTransmission* msg_data);

            /// \brief Receive incoming data from the %modem.
            ///
            /// If using one of the classes inheriting ModemDriverBase, this method should be bound and passed to ModemDriverBase::set_receive_cb.
            /// \param message The received ModemMessage.
            void handle_modem_receive(const protobuf::ModemDataTransmission& message);
        
            /// \brief Receive acknowledgements from the %modem.
            ///
            /// If using one of the classes inheriting ModemDriverBase, this method should be bound and passed to ModemDriverBase::set_ack_cb.
            /// \param message The ModemMessage corresponding to the acknowledgement (dest, src, frame#)
            void handle_modem_ack(const protobuf::ModemDataAck& message);
        
            //@}


            /// \name Active methods
            ///
            /// Call these methods when you want the QueueManager to perform time sensitive tasks
            //@{
            void do_work();

            //@}
        
            /// \name Informational Methods
            ///
            //@{        

            /// \return human readable summary of all loaded %queues
            std::string summary() const;
            const ManipulatorManager& manip_manager() const { return manip_manager_; }

            
            
            //@}
        
            /// \example libqueue/examples/queue_simple/queue_simple.cpp
            /// simple.xml
            /// \verbinclude queue_simple/simple.xml
            /// queue_simple.cpp


            /// \example acomms/examples/chat/chat.cpp

            static int modem_id_;

            boost::signal<void (const protobuf::ModemDataAck& ack_msg)> signal_ack;
            boost::signal<void (const protobuf::ModemDataTransmission& msg)> signal_receive;
            boost::signal<void (const protobuf::ModemDataTransmission& msg)> signal_receive_ccl;
            boost::signal<void (const protobuf::ModemDataExpire& expire_msg)> signal_expire;
            boost::signal<void (const protobuf::ModemDataRequest& request_msg,
                                protobuf::ModemDataTransmission* data_msg)> signal_data_on_demand;
            boost::signal<void (protobuf::QueueSize size)> signal_queue_size_change;
            
          private:
            /// \brief Add more %queues by configuration XML files (typically contained in DCCL message XML files).
            ///
            /// \param xml_file path to the XML file to parse and add to this codec.
            std::set<unsigned> add_xml_queue_file(const std::string& xml_file);

            /// \brief Add more Queues.
            ///
            /// \param QueueConfig& cfg: configuration object for the new %queue.
            void add_queue(const protobuf::QueueConfig& cfg);
        
            
            void qsize(Queue* q)
            {
                protobuf::QueueSize size;
                size.mutable_key()->CopyFrom(q->cfg().key());
                size.set_size(q->size());
                signal_queue_size_change(size);
            }
        
            // finds the %queue with the highest priority
            Queue* find_next_sender(const protobuf::ModemDataRequest& message, const protobuf::ModemDataTransmission& data_msg, bool first_user_frame);
        
            // combine multiple "user" frames into a single "modem" frame
            bool stitch_recursive(const protobuf::ModemDataRequest& request_msg, protobuf::ModemDataTransmission* data_msg, Queue* winning_var);
            bool unstitch_recursive(std::string* data, protobuf::ModemDataTransmission* message);

            void replace_header(bool is_last_user_frame, protobuf::ModemDataTransmission* data_msg, const protobuf::ModemDataTransmission& next_data_msg, const std::string& new_data);
       
            // clears the destination and ack values for the packet to reset for next $CADRQ
            void clear_packet();
            
            // slave function to receive_incoming_modem_data that actually writes a piece of the message (called for each user-frame)
            bool publish_incoming_piece(protobuf::ModemDataTransmission* message, const unsigned incoming_var_id);
            
        
            void process_cfg();
            
          private:
            std::map<goby::acomms::protobuf::QueueKey, Queue> queues_;
            
            std::ostream* log_;

            // map frame number onto %queue pointer that contains
            // the data for this ack
            std::multimap<unsigned, Queue*> waiting_for_ack_;

            // the first *user* frame sets the tone (dest & ack) for the entire packet (all %modem frames)
            unsigned packet_ack_;
            int packet_dest_;
 
            protobuf::QueueManagerConfig cfg_;
            
            ManipulatorManager manip_manager_;
        };

        /// outputs information about all available messages (same as std::string summary())
        std::ostream& operator<< (std::ostream& out, const QueueManager& d);

        
    }

}


#endif
