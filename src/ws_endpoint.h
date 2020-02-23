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

/*
* define a websocket server/client wrapper class
*/

#ifndef _WS_SVR_HANDLER_H_
#define _WS_SVR_HANDLER_H_

#include <iostream>
#include <vector>
#include <string>
#include <stdint.h>
#include "ws_packet.h"

typedef void (*nt_write_cb)(char * buf,int64_t size, void* wd);

class WebSocketEndpoint
{
public:
    WebSocketEndpoint( nt_write_cb write_cb);
    WebSocketEndpoint();
    virtual ~WebSocketEndpoint();

public:


    // start a websocket endpoint process. 
    virtual int32_t process(const char * readbuf, int32_t size);
    // start a websocket endpoint process. Also we register a write callback function
    virtual int32_t process(const char * readbuf, int32_t size, nt_write_cb write_cb, void* work_data);

    // receive data from wire until we get an entire handshake or frame data packet
    virtual int32_t from_wire(const char * readbuf, int32_t size);

    // try to find and parse a websocket packet
    virtual int64_t parse_packet(ByteBuffer& input);

    // process message data
    // users should rewrite this function 
    virtual int32_t process_message_data(WebSocketPacket& packet, ByteBuffer& frame_payload);

    // user defined process
    virtual int32_t user_defined_process(WebSocketPacket& packet, ByteBuffer& frame_payload);

    // send data to wire 
    virtual int32_t to_wire(const char * writebuf, int64_t size);

private:
    bool ws_handshake_completed_;

    ByteBuffer fromwire_buf_;
    ByteBuffer message_data_;

    nt_write_cb nt_write_cb_;
    void * nt_work_data_;

};
#endif//_WS_SVR_HANDLER_H_
