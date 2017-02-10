#ifndef SUNSHINE_UVCAST_H
#define SUNSHINE_UVCAST_H

#include "uv.h"
#include <type_traits>

struct UVHandle { typedef uv_handle_t type; };

struct UVAsync : UVHandle { typedef uv_async_t type; };
struct UVCheck : UVHandle { typedef uv_check_t type; };
struct UVFsEvent : UVHandle { typedef uv_fs_event_t type; };
struct UVFsPoll : UVHandle { typedef uv_fs_poll_t type; };
struct UVIdle : UVHandle { typedef uv_idle_t type; };
struct UVPoll : UVHandle { typedef uv_poll_t type; };
struct UVPrepare : UVHandle { typedef uv_prepare_t type; };
struct UVProcess : UVHandle { typedef uv_process_t type; };
struct UVUdp : UVHandle { typedef uv_udp_t type; };
struct UVTimer : UVHandle { typedef uv_timer_t type; };
struct UVSignal : UVHandle { typedef uv_signal_t type; };

struct UVStream : UVHandle { typedef uv_stream_t type; };
struct UVTcp : UVStream { typedef uv_tcp_t type; };
struct UVPipe : UVStream { typedef uv_pipe_t type; };
struct UVTty : UVStream { typedef uv_tty_t type; };


struct UVReq { typedef uv_req_t type; };

struct UVConnect : UVReq { typedef uv_connect_t type; };
struct UVWrite : UVReq { typedef uv_write_t type; };
struct UVShutdown : UVReq { typedef uv_shutdown_t type; };
struct UVUdpSend : UVReq { typedef uv_udp_send_t type; };
struct UVFs : UVReq { typedef uv_fs_t type; };
struct UVWork : UVReq { typedef uv_work_t type; };
struct UVGetaddrinfo : UVReq { typedef uv_getaddrinfo_t type; };
struct UVGetnameinfo : UVReq { typedef uv_getnameinfo_t type; };

template<typename T>
struct get_UV_type {
  typedef std::nullptr_t type;
};

#define make_get_UV_type(TYPE) template<>\
struct get_UV_type<TYPE::type> {\
  typedef TYPE type;\
}

make_get_UV_type(UVHandle);
make_get_UV_type(UVAsync);
make_get_UV_type(UVCheck);
make_get_UV_type(UVFsEvent);
make_get_UV_type(UVFsPoll);
make_get_UV_type(UVIdle);
make_get_UV_type(UVPoll);
make_get_UV_type(UVPrepare);
make_get_UV_type(UVProcess);
make_get_UV_type(UVUdp);
make_get_UV_type(UVTimer);
make_get_UV_type(UVSignal);
make_get_UV_type(UVStream);
make_get_UV_type(UVTcp);
make_get_UV_type(UVPipe);
make_get_UV_type(UVTty);
make_get_UV_type(UVReq);
make_get_UV_type(UVConnect);
make_get_UV_type(UVWrite);
make_get_UV_type(UVShutdown);
make_get_UV_type(UVUdpSend);
make_get_UV_type(UVFs);
make_get_UV_type(UVWork);
make_get_UV_type(UVGetaddrinfo);
make_get_UV_type(UVGetnameinfo);

template<typename T>
struct get_uv_type {
  static_assert(std::is_base_of<UVHandle, T>::value
                || std::is_base_of<UVReq, T>::value,
                "bad type");
  typedef typename T::type type;
};

template <typename F, typename T>
struct is_uv_upcast_safe {
  static_assert(!std::is_same<std::nullptr_t, typename get_UV_type<T>::type>::value, "invalid type");
  static constexpr bool value =
    std::is_base_of<
      typename get_UV_type<T>::type,
      typename get_UV_type<F>::type
    >::value;
};

template<typename T, typename F>
constexpr T* uv_upcast(F* from) {
  static_assert(is_uv_upcast_safe<F,T>::value, "invalid upcast");
  return reinterpret_cast<T*>(from);
};

#endif //SUNSHINE_UVCAST_H
