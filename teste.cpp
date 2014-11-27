
#include <uv.h>

#include <tuple>
#include <type_traits>
#include <iostream>

#include "eina_integer_sequence.hh"

template <typename F, typename R, typename Handle, typename...Args>
R do_call(Handle* handle, Args... args)
{
  return (*static_cast<F*>(handle->data))(handle, args...);
}

template <typename T>
struct find_function;

template <typename A>
struct find_function<std::tuple<A> >
{
  static constexpr const std::size_t value = 0;
};

template <typename A0, typename A1, typename...Args>
struct find_function<std::tuple<A0, A1, Args...> >
{
  static constexpr const std::size_t value =
    std::is_function<typename std::remove_pointer<A0>::type>::value ?
    0 : find_function<std::tuple<A1, Args...> >::value+1;
};

template <typename T>
struct identity { typedef T type; };

template <typename T>
struct function_parameter;

template <typename R, typename...Args>
struct function_parameter<R(*)(Args...)>
{
  typedef std::tuple<Args...> type;
};

template <typename T>
struct function_result;

template <typename R, typename...Args>
struct function_result<R(*)(Args...)>
{
  typedef R type;
};

template <typename C, typename R, typename... Params, typename H, typename F
          , std::size_t...S0, std::size_t...S1, typename...Args>
void call_with_func_object_impl2(F function, H* handle, efl::eina::index_sequence<S0...>
                                 , efl::eina::index_sequence<S1...>
                                 , std::tuple<Args...> tuple
                                 , identity<std::tuple<H*, Params...> >)
{
  function(handle, std::get<S0>(tuple)...
           ,
           static_cast<R(*)(H*, Params...)>
           (&::do_call<C, R, H, Params...>)
           , std::get<S1>(tuple)...
           );
}

template <std::size_t I, typename H, typename F, std::size_t...S0, std::size_t...S1, typename...Args>
void call_with_func_object_impl1(F function, H* handle, efl::eina::index_sequence<S0...> s0
                                 , efl::eina::index_sequence<S1...> s1
                                 , Args&& ... args)
{
  typedef std::tuple<Args...> tuple_type;
  tuple_type tuple (args...);
  typedef typename std::tuple_element<I-1, tuple_type>::type callback_type;
  handle->data = new callback_type(std::get<I-1>(tuple));

  typedef typename function_parameter<F>::type parameters;

  typedef typename std::tuple_element<I, parameters>::type cb_type;
  typedef typename function_parameter<cb_type>::type cb_parameters;
  typedef typename function_result<cb_type>::type cb_result;

  call_with_func_object_impl2<callback_type, cb_result>
    (function, handle, s0, s1, tuple, identity<cb_parameters>());
}

template <typename H, typename F, typename...Args>
void call_with_func_object(F function, H* handle, Args&& ... args)
{
  typedef typename function_parameter<F>::type parameters;
  static const std::size_t callback_index = find_function<parameters>::value;
  call_with_func_object_impl1<callback_index>
    (function,handle
     , efl::eina::make_index_sequence<callback_index-1>()
     , typename efl::eina::pop_integer_sequence
     <efl::eina::make_index_sequence<callback_index>
     , efl::eina::make_index_sequence<sizeof...(Args)> >::type()
     , std::forward<Args>(args)...);
}


int main()
{
  uv_poll_t handle;
  uv_poll_init(uv_default_loop(), &handle, 1);
  std::cout << "handle " << &handle << std::endl;
  call_with_func_object(uv_poll_start, &handle, UV_READABLE
                        , [] (uv_poll_t* h, int a1, int a2)
                        {
                          std::cout << "handle " << h << " a1 " << a1 << " a2 " << a2 << std::endl;
                        });
  uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}


