#include <stdlib.h>
#include <stdio.h>

#include <unordered_map>
#include <vector>
#include <string>

#include "uv.h"
#include "uvcast.h"

static uv_loop_t loop;
static std::unordered_map<uv_stream_t*, void*> ctx;

struct msg_md {
  size_t count = 0;
  std::string const* you_said = nullptr;
  std::string const* someone_said = nullptr;

  msg_md() {}

  msg_md(size_t _count, std::string const* _you,
                   std::string const* _someone) : count(_count),
                                                  you_said(_you),
                                                  someone_said(_someone) {}

  msg_md(msg_md&& rhs) {
    count = rhs.count;
    you_said = rhs.you_said;
    someone_said = rhs.someone_said;
    rhs.count = 0;
    rhs.you_said = nullptr;
    rhs.someone_said = nullptr;
  }

  ~msg_md() {
    if (you_said != nullptr) {
      delete you_said;
    }

    if (someone_said != nullptr) {
      delete someone_said;
    }
  }
};
static std::unordered_map<void*, msg_md> counts;



void destroy_stream_context(uv_handle_t *handle) noexcept {
  void* v;
  auto f = ctx.find(reinterpret_cast<uv_stream_t*>(handle));
  if (f == ctx.end()) {
    goto destroy_context_end;
  }

  v = f->second;
  ctx.erase(f);
  if (v != nullptr) {
    free(v);
  }
  destroy_context_end: {
    free(handle);
  }
}

void alloc_buffer(uv_handle_t *h, size_t suggested, uv_buf_t *buf) noexcept {
  std::ignore = h;
  buf->base = (char*) malloc(suggested);
  buf->len = suggested;
}

int main() {
  uv_loop_init(&loop);

  uv_tcp_t* tcp_server = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
  uv_tcp_init(&loop, tcp_server);

  struct sockaddr_in addr;
  uv_ip4_addr("127.0.0.1", 4444, &addr);

  uv_tcp_bind(tcp_server, (const struct sockaddr*)&addr, 0);

  int r = uv_listen(uv_upcast<uv_stream_t>(tcp_server), 10,
                    [](uv_stream_t *server, int status) {
    if (status < 0) {
      fprintf(stderr, "connection error: %s\n", uv_strerror(status));
      return;
    }

    uv_tcp_t *client_sock = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    uv_tcp_init(&loop, client_sock);

    if (uv_accept(server, uv_upcast<uv_stream_t>(client_sock)) == 0) {
      ctx[uv_upcast<uv_stream_t>(client_sock)] = nullptr;

      uv_read_start(uv_upcast<uv_stream_t>(client_sock), alloc_buffer,
                    [](uv_stream_t* sock, ssize_t nread, const uv_buf_t *buf) {
        if (nread < 0) {
          if (nread == UV_EOF) {
            uv_close(uv_upcast<uv_handle_t>(sock), destroy_stream_context);
          } else {
            fprintf(stderr, "read error %s\n", uv_err_name((int)nread));
            uv_close(uv_upcast<uv_handle_t>(sock), destroy_stream_context);
          }
          free(buf->base);
        } else if (nread > 0) {
          //do stuff
          std::vector<char>* data =
            new std::vector<char>(buf->base, buf->base + nread);
          free(buf->base);

          uv_buf_t wbuf[2];
          wbuf[1].base = data->data();
          wbuf[1].len = data->size();

          auto you = new std::string("you said: ");
          auto someone = new std::string("someone said: ");

          counts.emplace((void*)data, msg_md{ctx.size(), you, someone});

          for (auto const& kv: ctx) {
            if (kv.first == sock) {
              wbuf[0].base = const_cast<char*>(you->c_str());
              wbuf[0].len = you->size();
            } else {
              wbuf[0].base = const_cast<char*>(someone->c_str());
              wbuf[0].len = someone->size();
            }

            uv_write_t* wreq = (uv_write_t*)malloc(sizeof(uv_write_t));
            wreq->data = (void*)data;

            uv_write(wreq, kv.first, wbuf, 2, [](uv_write_t* req, int s) {
              std::ignore = s;
              auto& meta = counts[req->data];
              meta.count -= 1;
              if (meta.count  == 0) {
                counts.erase(req->data);
                auto d = (std::vector<char> *) req->data;
                delete d;
              }
              free(req);
            });
          }
        } else {
          //pass
        }
      });
    }
    else {
      uv_close(uv_upcast<uv_handle_t>(client_sock), destroy_stream_context);
    }
  });
  if (r == -1) {
    fprintf(stderr, "Listen error %s\n", uv_strerror(r));
    return -13;
  }

  uv_signal_t* sig = (uv_signal_t*)malloc(sizeof(uv_signal_t));

  uv_signal_init(&loop, sig);
  uv_signal_start(sig, [](uv_signal_t *sig_handle, int sig_num) {
    puts("\ngot sigint");
    std::ignore = sig_num;
    uv_idle_t* i = (uv_idle_t*)malloc(sizeof(uv_idle_t));
    uv_idle_init(&loop, i);
    uv_idle_start(i, [](uv_idle_t* handle){
      uv_idle_stop(handle);
      uv_close(uv_upcast<uv_handle_t>(handle), [](uv_handle_t* close_handle){
        free(close_handle);
      });

      uv_stop(&loop);
    });

    uv_signal_stop(sig_handle);
    uv_close(uv_upcast<uv_handle_t>(sig_handle), [](uv_handle_t* close_handle){
      free(close_handle);
    });
  }, SIGINT);

  puts("Listening...");
  uv_run(&loop, UV_RUN_DEFAULT);

  uv_walk(&loop, [](uv_handle_t* walk_handle, void* arg){
    std::ignore = arg;
    uv_close(walk_handle, [](uv_handle_t* close_handle) {
      free(close_handle);
    });
  }, nullptr);
  uv_run(&loop, UV_RUN_DEFAULT);

  int ret = uv_loop_close(&loop);
  if (ret != 0) {
    fprintf(stderr, "uv_loop_close error: %s\n", uv_strerror(ret));
  }

  return 0;
}
