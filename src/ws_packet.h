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

#ifndef _WS_PACKET_H_
#define _WS_PACKET_H_

#include <stdlib.h>
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <stdint.h>
#include "string_helper.h"

class ByteBuffer;

#define WS_ERROR_INVALID_HANDSHAKE_PARAMS 10070
#define WS_ERROR_INVALID_HANDSHAKE_FRAME 10071
// max handshake frame = 100k
#define WS_MAX_HANDSHAKE_FRAME_SIZE 1024 * 1000

/**
* a simple buffer class base on vector 
*/
class ByteBuffer
{
private:
    std::vector<char> data;

    // current offset in bytes from data.at(0) (data beginning)
    uint32_t oft;

public:
    ByteBuffer();
    virtual ~ByteBuffer();

public:
    /**
	* get the length of buffer. empty if zero.
	* @remark assert length() is not negative.
	*/
    virtual int length();
    /**
	* get the buffer bytes.
	* @return the bytes, NULL if empty.
	*/
    virtual char *bytes();
    /**
	* erase size of bytes from begin.
	* @param size to erase size of bytes from the beginning.
	*       clear if size greater than or equals to length()
	* @remark ignore size is not positive.
	*/
    virtual void erase(int size);
    /**
	* append specified bytes to buffer.
	* @param size the size of bytes
	* @remark assert size is positive.
	*/
    virtual void append(const char *bytes, int size);

    // resocman: exhance this class by adding thoes functions
    /** 
	* tell current position  return char * p=data.at(oft)
	*/
    virtual char *curat();
    /**
	* get current oft value
	*/
    virtual int getoft();
    /**
	* check if we have enough size in vector
	*/
    virtual bool require(int size);
    /**
	* move size bytes from cur position
	*/
    virtual bool skip_x(int size);
    /**
	*  read size bytes and move cur positon
	*/
    virtual bool read_bytes_x(char *cb, int size);
    /**
	* reset cur position to the beginning of vector
	*/
    virtual void resetoft();
};

class WebSocketPacket
{
public:
    WebSocketPacket();
    virtual ~WebSocketPacket(){};

public:
    enum WSPacketType : uint8_t
    {
        WSPacketType_HandShake = 1,
        WSPacketType_DataFrame,
    };

    enum WSOpcodeType : uint8_t
    {
        WSOpcode_Continue = 0x0,
        WSOpcode_Text = 0x1,
        WSOpcode_Binary = 0x2,
        WSOpcode_Close = 0x8,
        WSOpcode_Ping = 0x9,
        WSOpcode_Pong = 0xA,
    };

public:
    //
    /**
	* try to find and parse a handshake packet
    * @param input input buffer 
    * @param size size of input buffer
	* @return errcode, 0 means successful
    * 
	*/
    virtual int32_t recv_handshake(ByteBuffer &input);

    // fetch handshake element
    virtual int32_t fetch_hs_element(const std::string &msg);

    /**
	* pack a hand shake response packet
	* @return errcode
    * @param hs_rsp: a resp handshake packet, NULL if empty.
	*/
    virtual int32_t pack_handshake_rsp(std::string &hs_rsp);

    /**
    * try to find and parse a data frame
    * @return an entire frame size 
    * 0 means we need to continue recving data, 
    * and >0 means find get a frame successfule
    */
    virtual uint64_t recv_dataframe(ByteBuffer &input);

    /**
    * get frame info
    * @return header size
    */
    virtual int32_t fetch_frame_info(ByteBuffer &input);

    /**
    * get frame payload
    * @return only payload size
    */
    virtual int32_t fetch_payload(ByteBuffer &input);

    /**
    * pack a websocket data frame
    * @return 0 means successful
    */
    virtual int32_t pack_dataframe(ByteBuffer &input);

public:
    const uint8_t get_fin() { return fin_; }

    const uint8_t get_rsv1() { return rsv1_; }

    const uint8_t get_rsv2() { return rsv2_; }

    const uint8_t get_rsv3() { return rsv3_; }

    const uint8_t get_opcode() { return opcode_; }

    const uint8_t get_mask() { return mask_; }

    const uint64_t get_payload_length() { return payload_length_; }

    void set_fin(uint8_t fin) { fin_ = fin; }

    void set_rsv1(uint8_t rsv1) { rsv1_ = rsv1; }

    void set_rsv2(uint8_t rsv2) { rsv2_ = rsv2; }

    void set_rsv3(uint8_t rsv3) { rsv3_ = rsv3; }

    void set_opcode(uint8_t opcode) { opcode_ = opcode; }

    void set_mask(uint8_t mask) { mask_ = mask; }

    void set_payload_length(uint64_t length) { payload_length_ = length; }

    void set_payload(const char *buf, uint64_t size);

    const uint8_t get_length_type();

    const uint8_t get_header_size();

    ByteBuffer &get_payload() { return payload_; }

public:
    // get handshake packet length
    const uint32_t get_hs_length() { return hs_length_; }

public:
    const std::string mothod(void) const
    {
        return mothod_;
    }

    void mothod(const std::string &m)
    {
        mothod_ = m;
    }

    const std::string uri(void) const
    {
        return uri_;
    }

    void uri(const std::string &u)
    {
        uri_ = u;
    }

    const std::string version(void) const
    {
        return version_;
    }

    void version(const std::string &v)
    {
        version_ = v;
    }

    bool has_param(const std::string &name) const
    {
        return params_.find(name) != params_.end();
    }

    const std::string get_param(const std::string &name) const
    {
        std::map<std::string, std::string>::const_iterator it =
            params_.find(name);
        if (it != params_.end())
        {
            return it->second;
        }
        return std::string();
    }

    template <typename T>
    const T get_param(const std::string &name) const
    {
        return strHelper::valueOf<T, std::string>(get_param(name));
    }

    void set_param(const std::string &name, const std::string &v)
    {
        params_[name] = v;
    }

    template <typename T>
    void set_param(const std::string &name, const T &v)
    {
        params_[name] = strHelper::valueOf<std::string, T>(v);
    }

private:
    std::string mothod_;
    std::string uri_;
    std::string version_;
    std::map<std::string, std::string> params_;

private:
    uint8_t fin_;
    uint8_t rsv1_;
    uint8_t rsv2_;
    uint8_t rsv3_;
    uint8_t opcode_;
    uint8_t mask_;
    uint8_t length_type_;
    uint8_t masking_key_[4];
    uint64_t payload_length_;

private:
    uint32_t hs_length_;

public:
    ByteBuffer payload_;
};
#endif //_WS_PACKET_H_