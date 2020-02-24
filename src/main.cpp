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
* demostrate an asychronize websocket server base on websocketfiles 
* only use one working threadth
*
*/

#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include "uv.h"
#include "ws_endpoint.h"

#define DEFAULT_BACKLOG 128
// for each connected client.
typedef struct
{
  uv_tcp_t *uvclient;
  WebSocketEndpoint *endpoint;
} peer_state_t;

typedef struct
{
  uv_tcp_t *uvclient;
  WebSocketEndpoint *endpoint;
  uv_buf_t request;
  uv_buf_t response;
  int type;
} peer_work_data_t;

void fail(char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fprintf(stderr, "\n");
  exit(EXIT_FAILURE);
}

void* xmalloc(size_t size) {
  void* ptr = malloc(size);
  if (!ptr) {
    fail("malloc failed");
  }
  return ptr;
}

void on_alloc_buffer(uv_handle_t *handle, size_t suggested_size,
                     uv_buf_t *buf)
{
  buf->base = (char *)xmalloc(suggested_size);
  buf->len = suggested_size;
}

void on_alloc_buffer_v2(char *buf, uint64_t suggested_size)
{
  buf = (char *)xmalloc(suggested_size);
}

int uv_buf_alloc_cpy(uv_buf_t *dst, const uv_buf_t *src, int size)
{
  if (src == NULL || dst == NULL)
  {
    printf("main - uv buffer alloc and copy failed[src prt is NULL]!\r\n");
    return -1;
  }

  if (src->len < size)
  {
    printf("main - uv buffer alloc and copy failed[src-len < size]!\r\n");
    return -2;
  }

  dst->base = (char *)xmalloc(size);
  dst->len = size;

  memcpy(dst->base, src->base, size);
}

void set_thread_pool_size()
{
  int nthread = 0;
  const char *val = getenv("UV_THREADPOOL_SIZE");
  if (val != NULL)
  {
    nthread = atoi(val);
    printf("dafault thread pool size:%d\n", nthread);
  }
  setenv("UV_THREADPOOL_SIZE", "1", 1);

  val = getenv("UV_THREADPOOL_SIZE");
  nthread = atoi(val);
  printf("set thread pool size:%d\n", nthread);
}

void free_work_data(peer_work_data_t *workdata)
{
  //not free endpoint
  workdata->uvclient = NULL;
  workdata->endpoint = NULL;
  free(workdata->request.base);
  workdata->request.base = NULL;
  free(workdata->response.base);
  workdata->response.base = NULL;
  free(workdata);
}

void on_write_response(char *buf, int64_t size, void *wd)
{
  peer_work_data_t *work_data = (peer_work_data_t *)wd;
  if (work_data->response.base != NULL)
  {
    fail("Work data response is not NULL!");
  }

  uv_buf_t src = uv_buf_init(buf, size);
  work_data->response = uv_buf_init(NULL, 0);
  uv_buf_alloc_cpy(&(work_data->response), &src, src.len);
}

void on_client_closed(uv_handle_t *handle)
{
  uv_tcp_t *client = (uv_tcp_t *)handle;

  // we free client data it here.
  if (client->data)
  {
    peer_state_t *peerstate = (peer_state_t *)client->data;
    delete peerstate->endpoint;
    peerstate->endpoint = NULL;
    free(client->data);
  }
  free(client);
}

void on_work_client_closed(uv_work_t *req)
{
  if (req == NULL)
  {
    printf("main - on_client_close: req is null\r\n");
    return;
  }

  peer_work_data_t *work_data = (peer_work_data_t *)req->data;
  peer_state_t *peerstate = (peer_state_t *)work_data->uvclient->data;

  delete work_data->endpoint;
  work_data->endpoint = NULL;
  free((peer_state_t *)work_data->uvclient->data);
  free(work_data->uvclient);
  free(work_data->request.base);
  free(work_data->response.base);
  free(work_data);
  free(req);
}

void on_sent_response(uv_write_t *req, int status)
{
  if (status)
  {
    fail("Write error: %s\n", uv_strerror(status));
  }

  peer_work_data_t *work_data = (peer_work_data_t *)req->data;
  free_work_data(work_data);
  free(req);
}

// Runs in a separate thread, can do blocking/time-consuming operations.
void on_work_submitted(uv_work_t *req)
{
  peer_work_data_t *work_data = (peer_work_data_t *)req->data;

  int nrc = work_data->endpoint->process(work_data->request.base, work_data->request.len,
                                         on_write_response, work_data);
  if (nrc < 0)
  {
    printf("main - process read buf failed with[err:%d].\r\n", nrc);
  }
}

void on_work_completed(uv_work_t *req, int status)
{
  if (status)
  {
    fail("on_work_completed error: %s\n", uv_strerror(status));
  }

  peer_work_data_t *work_data = (peer_work_data_t *)req->data;

  if (work_data->response.base == NULL)
  {
    printf("main - no response data! we will free work data and return directly!\r\n");
    work_data->endpoint = NULL;
    work_data->uvclient = NULL;
    free_work_data(work_data);
    free(req);
    return;
  }

  //printf("main - on_work_completed client adr:0x%x \r\n",work_data->uvclient);
  uv_write_t *writereq = (uv_write_t *)xmalloc(sizeof(*writereq));
  writereq->data = req->data;
  req->data = NULL;

  int rc;
  if ((rc = uv_write(writereq, (uv_stream_t *)work_data->uvclient, &(work_data->response), 1,
                     on_sent_response)) < 0)
  {
    fail("uv_write failed: %s", uv_strerror(rc));
  }
  free(req);
}

uv_work_t *alloc_work_req(peer_state_t *peerstate, ssize_t nread, const uv_buf_t *buf, int type)
{
  uv_work_t *work_req = (uv_work_t *)xmalloc(sizeof(*work_req));
  peer_work_data_t *work_data = (peer_work_data_t *)xmalloc(sizeof(*work_data));
  work_data->uvclient = peerstate->uvclient;
  work_data->endpoint = peerstate->endpoint;
  work_data->request = uv_buf_init(NULL, 0);
  work_data->type = type;
  if (work_data->type == 1)
  {
    if (uv_buf_alloc_cpy(&(work_data->request), buf, nread) < 0)
    {
      free_work_data(work_data);
      free(buf->base);
      return NULL;
    }
  }
  work_data->response = uv_buf_init(NULL, 0);
  work_req->data = work_data;
  return work_req;
}

void on_peer_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf)
{
  int rc = 0;
  if (nread < 0)
  {
    if (nread != UV_EOF)
    {
      fprintf(stderr, "Read error: %s\n", uv_strerror(nread));
    }

    peer_state_t *peerstate = (peer_state_t *)client->data;
    peerstate->uvclient = (uv_tcp_t *)client;
    uv_work_t *work_req = alloc_work_req(peerstate, nread, buf, 0);
    if ((rc = uv_queue_work(uv_default_loop(), work_req, on_work_client_closed, NULL)) < 0)
    {
      fail("uv_queue_work failed: %s", uv_strerror(rc));
    }
  }
  else if (nread == 0)
  {
    // From the documentation of uv_read_cb: nread might be 0, which does not
    // indicate an error or EOF. This is equivalent to EAGAIN or EWOULDBLOCK
    // under read(2).
    printf("main - on_peer_read: nread==0 !\r\n");
  }
  else
  {
    // nread > 0
    assert(buf->len >= nread);

    peer_state_t *peerstate = (peer_state_t *)client->data;
    //printf("main - on_peer_read: Watch ps-client:0x%x, client: 0x%x, ps-client-data:0x%x, client-data:0x%x\n",
     //      peerstate->uvclient, client, peerstate->uvclient->data,client->data);
    peerstate->uvclient = (uv_tcp_t *)client;

    // add work reqs on the work queue, without blocking the
    // callback.
    uv_work_t *work_req = alloc_work_req(peerstate, nread, buf, 1);
    if ((rc = uv_queue_work(uv_default_loop(), work_req, on_work_submitted,
                            on_work_completed)) < 0)
    {
      fail("uv_queue_work failed: %s", uv_strerror(rc));
    }
  }
  free(buf->base);
}

void report_peer_connected(const struct sockaddr_in* sa, socklen_t salen) {
  char hostbuf[NI_MAXHOST];
  char portbuf[NI_MAXSERV];
  if (getnameinfo((struct sockaddr*)sa, salen, hostbuf, NI_MAXHOST, portbuf,
                  NI_MAXSERV, 0) == 0) {
    printf("peer (%s, %s) connected\n", hostbuf, portbuf);
  } else {
    printf("peer (unknonwn) connected\n");
  }
}

void on_peer_connected(uv_stream_t *server, int status)
{
  if (status < 0)
  {
    fprintf(stderr, "Peer connection error: %s\n", uv_strerror(status));
    return;
  }

  // client represents this peer and we will
  // release it when the client disconnects. 
  uv_tcp_t *client = (uv_tcp_t *)xmalloc(sizeof(*client));
  int rc;
  if ((rc = uv_tcp_init(uv_default_loop(), client)) < 0)
  {
    fail("uv_tcp_init failed: %s", uv_strerror(rc));
  }
  client->data = NULL;

  if (uv_accept(server, (uv_stream_t *)client) == 0)
  {
    struct sockaddr_storage peername;
    int namelen = sizeof(peername);
    if ((rc = uv_tcp_getpeername(client, (struct sockaddr *)&peername,
                                 &namelen)) < 0)
    {
      fail("uv_tcp_getpeername failed: %s", uv_strerror(rc));
    }
    report_peer_connected((const struct sockaddr_in *)&peername, namelen);

    peer_state_t *peerstate = (peer_state_t *)xmalloc(sizeof(*peerstate));
    peerstate->endpoint = new WebSocketEndpoint();
    peerstate->uvclient = client;
    client->data = peerstate;

    if ((rc = uv_read_start((uv_stream_t *)client, on_alloc_buffer,
                            on_peer_read)) < 0)
    {
      fail("uv_read_start failed: %s", uv_strerror(rc));
    }
  }
  else
  {
    uv_close((uv_handle_t *)client, on_client_closed);
  }
}

int main(int argc, const char **argv)
{
  setvbuf(stdout, NULL, _IONBF, 0);

  int portnum = 9000;
  if (argc >= 2)
  {
    portnum = atoi(argv[1]);
  }
  printf("Serving on port %d\n", portnum);

  int rc;
  uv_tcp_t server;
  if ((rc = uv_tcp_init(uv_default_loop(), &server)) < 0)
  {
    fail("uv_tcp_init failed: %s", uv_strerror(rc));
  }

  struct sockaddr_in addr;
  if ((rc = uv_ip4_addr("0.0.0.0", portnum, &addr)) < 0)
  {
    fail("uv_ip4_addr failed: %s", uv_strerror(rc));
  }

  if ((rc = uv_tcp_bind(&server, (const struct sockaddr *)&addr, 0)) < 0)
  {
    fail("uv_tcp_bind failed: %s", uv_strerror(rc));
  }

  // Listen on the socket for new peers to connect. When a new peer connects,
  // the on_peer_connected callback will be invoked.
  if ((rc = uv_listen((uv_stream_t *)&server, DEFAULT_BACKLOG, on_peer_connected)) <
      0)
  {
    fail("uv_listen failed: %s", uv_strerror(rc));
  }

  //printf("main - main: set thread pool size.\r\n");
  set_thread_pool_size();
  // Run the libuv event loop.
  uv_run(uv_default_loop(), UV_RUN_DEFAULT);

  // If uv_run returned, close the default loop before exiting.
  return uv_loop_close(uv_default_loop());
}
