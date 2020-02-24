# Websocketfiles(1.02)  

## Introduction  

Websocketfiles provides two well designed and out of box websocket c++ classes that you can easily use in your ongoing project which needs to support websocket.  

The purpose of this project is to let you add websocket support in your c++ project as quickly/efficient as possible.The websocketfiles is designed as simple as possible and no network transport module included. It exports two network interfaces named from_wire/to_wire that can be easily binding with any network modules(Boost.Asio, libuv, libevent and so on).  

An asynchronous websocket server using libuv is provided to demostrate how to use websocketfiles in a C++ project.  

## Features  
  
  * **An out of box and light weight websocket c++ classes**(1000+ lines of C++98 code).It is well designed and tested and easily to merge into your ongoing c++ project(or some old c++ projects).  
  * **Support RFC6455**  
  * **No network transport modules included.**As people may have different network transport modules in their projects, websocketfiles only fouces on packing/unpacking websocket packet.  
  * **Multi-platform support(linux/windows)**  
  * **Fully traced websocket message flow.**A fully tracing infomation of websocket message helps you know websocketfiles code rapidly and expand funcions easily.  
  
## Class and file overview  
  
  1. Class WebsocketPacket: a websocket packet class  
  2. Class WebsocketEndpoint: a websocket server/client wrapper class  
  3. Class strHelper: a string operation class for parsing websocket handshake message   
  4. Class ByteBuffer: a simple buffer class base on vector  
  5. File sha1.cpp and base64.cpp: SHA1 and base64 encode/decode functions for masking/unmasking data  
  6. File main.cpp: provide an asynchronous websocket server demonstration using libuv as netork transport.  
  7. Folder src: source file(websocketfiles source code)  
  8. Folder include: libuv include files(only for demo)  
  9. Folder lib: libuv so file(only for demo)  
  
## How to use it in your project  
  
Copy all files except main.cpp from src folder to your project folder. Modify function WebSocketEndpoint::from_wire/to_wire and combine it with your network transport read/write function.The module-connection looks like below:  

![Alt text](https://github.com/beikesong/websocketfiles/blob/master/image/module-connection.png)  
  
## Building and testing  
  
```bash
git clone --recurive  
cd websocketfiles  
make  
./wsfiles_server_uv.1.02  
```
  
Attention: The asynchronous demo wsfiles_server_uv only uses an event thread and a single working thread(based on libuv).If you want to increase the number of working thread more than 1 (default value is 1), you can modify UV_THREADPOOL_SIZE value. The most important thing is that you must add some protection codes in main.cpp to make sure some variables are thread safe in multi-working-thread situation.  
  
Start wsfiles_server_uv, and we get tracing messages on console:  

```bash
set thread pool size:1  
peer (xxx.xxx.xxx.xxx, 11464) connected  
handshake element k:Host v: yyy.yyy.yyy.yyy:9050  
handshake element k:Connection v:Upgrade  
handshake element k:Pragma v:no-cache  
handshake element k:Cache-Control v:no-cache  
handshake element k:User-Agent v:Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko)   Chrome/79.0.3945.130 Safari/537.36  
handshake element k:Upgrade v:websocket  
handshake element k:Origin v:http://www.bejson.com  
handshake element k:Sec-WebSocket-Version v:13  
handshake element k:Accept-Encoding v:gzip, deflate  
handshake element k:Accept-Language v:zh-CN,zh;q=0.9  
handshake element k:Sec-WebSocket-Key v:lEecdWuXh4ekgX/oWBSc8A==  
handshake element k:Sec-WebSocket-Extensions v:permessage-deflate; client_max_window_bits  
WebsocketEndpont - handshake successful!  

WebSocketPacket: received data with header size: 6 payload size:28 input oft size:34  
WebSocketEndpoint - recv a Text opcode.  
WebSocketEndpoint - received data, length:28 ,content:first websocket test message  
WebSocketPacket: send data with header size: 2 payload size:28  
```
  
  
After a successful websocket handshake, the demo server will return the message that you send to it. It is easily to change response message by modify function WebSocketEndpoint::process_message_data.  
  
```cpp
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
        ...  
        ... 
```
  
## Future  

1.Add more demos to illustrate how to use websocketfiles with Boot.Asio and libevent.  
2.Support TLS.   

 
