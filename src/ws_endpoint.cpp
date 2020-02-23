/*
* The MIT License (MIT)
* Copyright(c) 2020 BeikeSong

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "ws_endpoint.h"
#include "markupstl.h"

WebSocketEndpoint::WebSocketEndpoint()
{
    //networklayer_ = nt;
    nt_write_cb_ = NULL;
    ws_handshake_completed_ = false;
}

WebSocketEndpoint::WebSocketEndpoint(nt_write_cb write_cb)
{
    //networklayer_ = nt;
    nt_write_cb_ = write_cb;
    ws_handshake_completed_ = false;
}

WebSocketEndpoint::~WebSocketEndpoint() {}

int32_t WebSocketEndpoint::process(const char *readbuf, int32_t size)
{
    return from_wire(readbuf, size);
}

int32_t WebSocketEndpoint::process(const char *readbuf, int32_t size, nt_write_cb write_cb, void *work_data)
{
    if (write_cb == NULL || work_data == NULL)
    {
        std::cout << "WebSocketEndpoint - Attention: write cb is NULL! It will skip current read buf!" << std::endl;
        return 0;
    }

    nt_write_cb_ = write_cb;
    nt_work_data_ = work_data;

    return from_wire(readbuf, size);
}

int32_t WebSocketEndpoint::from_wire(const char *readbuf, int32_t size)
{
    fromwire_buf_.append(readbuf, size);
    while (true)
    {
        int64_t nrcv = parse_packet(fromwire_buf_);
        if (nrcv > 0)
        { // for next one
            // clear used data
            int64_t n_used = fromwire_buf_.getoft();
            //std::cout<< "WebSocketEndpoint - fromwire_buf: used data:"<<n_used <<" nrcv:"<<nrcv
            //    <<" length:"<<fromwire_buf_.length()<<std::endl;
            fromwire_buf_.erase(nrcv);
            fromwire_buf_.resetoft();
            if (fromwire_buf_.length() == 0)
            {
                return nrcv;
            }
            else
            {
                continue;
            }
        }
        else if (nrcv == 0)
        { // contueue recving
            fromwire_buf_.resetoft();
            break;
        }
        else
        {
            return -1;
        }
    }

    // make it happy
    return 0;
}

int32_t WebSocketEndpoint::to_wire(const char *writebuf, int64_t size)
{
    //networklayer_->toWire(writebuf,size);
    if (nt_write_cb_ == NULL || nt_work_data_ == NULL || writebuf == NULL || size <= 0)
    {
        return 0;
    }

    nt_write_cb_(const_cast<char *>(writebuf), size, nt_work_data_);
    return 0;
}

int64_t WebSocketEndpoint::parse_packet(ByteBuffer &input)
{
    WebSocketPacket wspacket;
    if (!ws_handshake_completed_)
    {
        uint32_t nstatus = 0;
        nstatus = wspacket.recv_handshake(input);
        if (nstatus != 0)
        {
            return -1;
        }

        if (wspacket.get_hs_length() == 0)
        {
            // not enough data for a handshake message
            // continue recving data
            return 0;
        }

        std::string hs_rsp;
        wspacket.pack_handshake_rsp(hs_rsp);
        to_wire(hs_rsp.c_str(), hs_rsp.length());
        ws_handshake_completed_ = true;
        std::cout << "WebsocketEndpont - handshake successful!" << std::endl
                  << std::endl;

        return wspacket.get_hs_length();
    }
    else
    {
        uint64_t ndf = wspacket.recv_dataframe(input);

        // continue recving data until get an entire frame
        if (ndf == 0)
        {
            return 0;
        }

        if (ndf > 0xFFFFFFFF)
        {
            std::cout << "Attention:frame data length exceeds the max value of a uint32_t varable!" << std::endl;
        }

        ByteBuffer &payload = wspacket.get_payload();
        message_data_.append(payload.bytes(), payload.length());

        // now, we have a entire frame
        if (wspacket.get_fin() == 1)
        {
            process_message_data(wspacket, message_data_);
            message_data_.erase(message_data_.length());
            message_data_.resetoft();
            return ndf;
        }

        return ndf;
    }

    return -1;
}

int32_t WebSocketEndpoint::process_message_data(WebSocketPacket &packet, ByteBuffer &frame_payload)
{
    //#ifdef _SHOW_OPCODE_
    switch (packet.get_opcode())
    {
    case WebSocketPacket::WSOpcode_Continue:
        // add your process code here
        std::cout << "WebSocketEndpoint - recv a Continue opcode." << std::endl;
        user_defined_process(packet, frame_payload);
        break;
    case WebSocketPacket::WSOpcode_Text:
        // add your process code here
        std::cout << "WebSocketEndpoint - recv a Text opcode." << std::endl;
        user_defined_process(packet, frame_payload);
        break;
    case WebSocketPacket::WSOpcode_Binary:
        // add your process code here
        std::cout << "WebSocketEndpoint - recv a Binary opcode." << std::endl;
        user_defined_process(packet, frame_payload);
        break;
    case WebSocketPacket::WSOpcode_Close:
        // add your process code here
        std::cout << "WebSocketEndpoint - recv a Close opcode." << std::endl;
        user_defined_process(packet, frame_payload);
        break;
    case WebSocketPacket::WSOpcode_Ping:
        // add your process code here
        std::cout << "WebSocketEndpoint - recv a Ping opcode." << std::endl;
        user_defined_process(packet, frame_payload);
        break;
    case WebSocketPacket::WSOpcode_Pong:
        // add your process code here
        std::cout << "WebSocketEndpoint - recv a Pong opcode." << std::endl;
        user_defined_process(packet, frame_payload);
        break;
    default:
        std::cout << "WebSocketEndpoint - recv an unknown opcode." << std::endl;
        break;
    }
    //#endif
    return 0;
}

// we directly return what we get from client
// user could modify this function
int32_t WebSocketEndpoint::user_defined_process(WebSocketPacket &packet, ByteBuffer &frame_payload)
{
    // print received websocket payload from client
    std::string str_recv(frame_payload.bytes(), frame_payload.length());
    std::cout << "WebSocketEndpoint - received data, length:" << str_recv.length()
              << " ,content:" << str_recv.c_str() << std::endl<< std::endl;

    WebSocketPacket wspacket;
    // set FIN and opcode
    wspacket.set_fin(1);
    wspacket.set_opcode(packet.get_opcode());
    // set payload data
    wspacket.set_payload(frame_payload.bytes(), frame_payload.length());
    ByteBuffer output;
    // pack a websocket data frame
    wspacket.pack_dataframe(output);
    // send to client
    to_wire(output.bytes(), output.length());
}
