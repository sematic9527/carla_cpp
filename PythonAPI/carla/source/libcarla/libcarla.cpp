// Copyright (c) 2019 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include <carla/Memory.h>
#include <carla/PythonUtil.h>
#include <carla/Time.h>

#include <ostream>
// 类型萃取，定义了一系列的类模板，用于获取类型
// 可以用来在编译期判断类型的属性、对给定类型进行一些操作获得另一种特定类型、判断类型和类型之间的关系等
#include <type_traits> 
#include <vector>

// 对于Python中的变量类型，Boost.Python都有相应的类对应，他们都是boost::python::object的子类。
// boost::python::object 包装了PyObject *, 通过这种方式，C++可以平滑的操作python对象。
// Boost.Python的主要目标既保持Python的编程风格同时又提供C++和Python的双向映射。
template <typename OptionalT>
static boost::python::object OptionalToPythonObject(OptionalT &optional) {
  // 如果 optional 有值，将其转换为 boost::python::object，否则返回一个空的 boost::python::object
  return optional.has_value() ? boost::python::object(*optional) : boost::python::object();
}

// 方便地进行没有参数的请求。
// carla::PythonUtil::ReleaseGIL 文档：https://carla.org/Doxygen/html/d1/d0a/classcarla_1_1PythonUtil_1_1ReleaseGIL.html
// GIL的全称为Global Interpreter Lock，全局解释器锁
// GIL(全局解释锁)不是Python的特性，而是解释器CPtyhton的特性。
// GIL简言之是一个互斥锁，只允许一个线程控制 Python 解释器，也就是说，在任一时间点都只有一个线程处于执行状态。
#define CALL_WITHOUT_GIL(cls, fn) +[](cls &self) { \
      carla::PythonUtil::ReleaseGIL unlock; \
      return self.fn(); \
    }

// 方便地进行带有1个参数的请求。
// std::forward 主要用于完美转发，能够保留传递给函数参数的值类别（lvalue 或 rvalue），确保在转发参数时不丢失其原有的值性质。
#define CALL_WITHOUT_GIL_1(cls, fn, T1_) +[](cls &self, T1_ t1) { \
      carla::PythonUtil::ReleaseGIL unlock; \
      return self.fn(std::forward<T1_>(t1)); \
    }

// 方便地进行带有2个参数的请求。
#define CALL_WITHOUT_GIL_2(cls, fn, T1_, T2_) +[](cls &self, T1_ t1, T2_ t2) { \
      carla::PythonUtil::ReleaseGIL unlock; \
      return self.fn(std::forward<T1_>(t1), std::forward<T2_>(t2)); \
    }

// 方便地进行带有3个参数的请求。
#define CALL_WITHOUT_GIL_3(cls, fn, T1_, T2_, T3_) +[](cls &self, T1_ t1, T2_ t2, T3_ t3) { \
      carla::PythonUtil::ReleaseGIL unlock; \
      return self.fn(std::forward<T1_>(t1), std::forward<T2_>(t2), std::forward<T3_>(t3)); \
    }

// 方便地进行带有4个参数的请求。
#define CALL_WITHOUT_GIL_4(cls, fn, T1_, T2_, T3_, T4_) +[](cls &self, T1_ t1, T2_ t2, T3_ t3, T4_ t4) { \
      carla::PythonUtil::ReleaseGIL unlock; \
      return self.fn(std::forward<T1_>(t1), std::forward<T2_>(t2), std::forward<T3_>(t3), std::forward<T4_>(t4)); \
    }

// 方便地进行带有5个参数的请求。
#define CALL_WITHOUT_GIL_5(cls, fn, T1_, T2_, T3_, T4_, T5_) +[](cls &self, T1_ t1, T2_ t2, T3_ t3, T4_ t4, T5_ t5) { \
      carla::PythonUtil::ReleaseGIL unlock; \
      return self.fn(std::forward<T1_>(t1), std::forward<T2_>(t2), std::forward<T3_>(t3), std::forward<T4_>(t4), std::forward<T5_>(t5)); \
    }

// 方便地进行没有参数的常量请求。
#define CONST_CALL_WITHOUT_GIL(cls, fn) CALL_WITHOUT_GIL(const cls, fn)
#define CONST_CALL_WITHOUT_GIL_1(cls, fn, T1_) CALL_WITHOUT_GIL_1(const cls, fn, T1_)
#define CONST_CALL_WITHOUT_GIL_2(cls, fn, T1_, T2_) CALL_WITHOUT_GIL_2(const cls, fn, T1_, T2_)
#define CONST_CALL_WITHOUT_GIL_3(cls, fn, T1_, T2_, T3_) CALL_WITHOUT_GIL_3(const cls, fn, T1_, T2_, T3_)
#define CONST_CALL_WITHOUT_GIL_4(cls, fn, T1_, T2_, T3_, T4_) CALL_WITHOUT_GIL_4(const cls, fn, T1_, T2_, T3_, T4_)

// 方便用于需要复制返回值的const请求。 
// cls：类名class
// fn: 函数名function name
// 例如：CALL_RETURNING_COPY(cc::Actor, GetWorld)
// decltype 类型说明符生成指定表达式的类型（用于进行编译时类型推导）
// std::result_of_t 用于获取函数对象调用后的返回类型。
// 它的用法是 std::result_of_t<F(Args...)>，其中 F是函数对象类型，Args...是函数参数类型列表。
// std::decay_t 是一个类型转换工具，它可以帮助我们处理类型退化（decay）的情况
// +[](const cls &self) -> std::decay_t<std::result_of_t<decltype(&cls::fn)(cls*)>> { return self.fn(); }
#define CALL_RETURNING_COPY(cls, fn) +[](const cls &self) \
        -> std::decay_t<std::result_of_t<decltype(&cls::fn)(cls*)>> { \
      return self.fn(); \
    }

// 方便用于需要复制返回值的const请求。
#define CALL_RETURNING_COPY_1(cls, fn, T1_) +[](const cls &self, T1_ t1) \
        -> std::decay_t<std::result_of_t<decltype(&cls::fn)(cls*, T1_)>> { \
      return self.fn(std::forward<T1_>(t1)); \
    }
// 将 Python 的列表类型 (boost::python::list) 转换为 C++ 的 std::vector 类型
template<typename T>
  // 定义一个 C++ 的 std::vector 容器，用于存储转换后的数据

std::vector<T> PythonLitstToVector(boost::python::list &input) {
  std::vector<T> result;
    // 获取 Python 列表的长度（元素个数）

  boost::python::ssize_t list_size = boost::python::len(input);
    // 遍历 Python 列表的每个元素

  for (boost::python::ssize_t i = 0; i < list_size; ++i) {
        // 从 Python 列表中提取对应索引的元素，转换为模板类型 T 并添加到 std::vector 中

    result.emplace_back(boost::python::extract<T>(input[i]));
  }
  return result;
}

// 方便地需要将返回值转换为Python列表的const请求。
#define CALL_RETURNING_LIST(cls, fn) +[](const cls &self) { \
      boost::python::list result; \
      for (auto &&item : self.fn()) { \
        result.append(item); \
      } \
      return result; \
    }

// 方便需要将返回值转换为Python列表的const请求。
#define CALL_RETURNING_LIST_1(cls, fn, T1_) +[](const cls &self, T1_ t1) { \
      boost::python::list result; \
      for (auto &&item : self.fn(std::forward<T1_>(t1))) { \
        result.append(item); \
      } \
      return result; \
    }

#define CALL_RETURNING_LIST_2(cls, fn, T1_, T2_) +[](const cls &self, T1_ t1, T2_ t2) { \
      boost::python::list result; \
      for (auto &&item : self.fn(std::forward<T1_>(t1), std::forward<T2_>(t2))) { \
        result.append(item); \
      } \
      return result; \
    }

#define CALL_RETURNING_LIST_3(cls, fn, T1_, T2_, T3_) +[](const cls &self, T1_ t1, T2_ t2, T3_ t3) { \
      boost::python::list result; \
      for (auto &&item : self.fn(std::forward<T1_>(t1), std::forward<T2_>(t2), std::forward<T3_>(t3))) { \
        result.append(item); \
      } \
      return result; \
    }

#define CALL_RETURNING_OPTIONAL(cls, fn) +[](const cls &self) { \
      auto optional = self.fn(); \
      return OptionalToPythonObject(optional); \
    }

#define CALL_RETURNING_OPTIONAL_1(cls, fn, T1_) +[](const cls &self, T1_ t1) { \
      auto optional = self.fn(std::forward<T1_>(t1)); \
      return OptionalToPythonObject(optional); \
    }

#define CALL_RETURNING_OPTIONAL_2(cls, fn, T1_, T2_) +[](const cls &self, T1_ t1, T2_ t2) { \
      auto optional = self.fn(std::forward<T1_>(t1), std::forward<T2_>(t2)); \
      return OptionalToPythonObject(optional); \
    }

#define CALL_RETURNING_OPTIONAL_3(cls, fn, T1_, T2_, T3_) +[](const cls &self, T1_ t1, T2_ t2, T3_ t3) { \
      auto optional = self.fn(std::forward<T1_>(t1), std::forward<T2_>(t2), std::forward<T3_>(t3)); \
      return OptionalToPythonObject(optional); \
    }

#define CALL_RETURNING_OPTIONAL_WITHOUT_GIL(cls, fn) +[](const cls &self) { \
      auto call = CONST_CALL_WITHOUT_GIL(cls, fn); \
      auto optional = call(self); \
      return optional.has_value() ? boost::python::object(*optional) : boost::python::object(); \
    }

template <typename T>
static void PrintListItem_(std::ostream &out, const T &item) {
  // 将元素输出到输出流
  out << item;
}

template <typename T>
static void PrintListItem_(std::ostream &out, const carla::SharedPtr<T> &item) {
  // 如果指针为空，输出 nullptr
  if (item == nullptr) {
    out << "nullptr";
  } else {
    // 否则输出指针所指向的内容
    out << *item;
  }
}

template <typename Iterable>
static std::ostream &PrintList(std::ostream &out, const Iterable &list) {
  out << '[';
  if (!list.empty()) {
    auto it = list.begin();
    PrintListItem_(out, *it);
    for (++it; it != list.end(); ++it) {
      out << ", ";
      PrintListItem_(out, *it);
    }
  }
  out << ']';
  return out;
}

namespace std {

  template <typename T>
  std::ostream &operator<<(std::ostream &out, const std::vector<T> &vector_of_stuff) {
    // 重载输出流操作符，将 vector 元素输出为列表形式
    return PrintList(out, vector_of_stuff);
  }

  template <typename T, typename H>
  std::ostream &operator<<(std::ostream &out, const std::pair<T,H> &data) {
    // 重载输出流操作符，将 pair 元素输出为 (first,second) 形式
    out << "(" << data.first << "," << data.second << ")";
    return out;
  }

} // namespace std

static carla::time_duration TimeDurationFromSeconds(double seconds) {
  size_t ms = static_cast<size_t>(1e3 * seconds);
  // 将秒转换为毫秒，并创建时间持续时间对象
  return carla::time_duration::milliseconds(ms);
}

static auto MakeCallback(boost::python::object callback) {
  namespace py = boost::python;
  // 确保回调实际上是可调用的。
  if (!PyCallable_Check(callback.ptr())) {
    PyErr_SetString(PyExc_TypeError, "callback argument must be callable!");
    py::throw_error_already_set();
  }

  // 我们需要在持有GIL的同时删除回调。
  using Deleter = carla::PythonUtil::AcquireGILDeleter;
  auto callback_ptr = carla::SharedPtr<py::object>{new py::object(callback), Deleter()};

  // 做一个lambda回调。
  return [callback=std::move(callback_ptr)](auto message) {
    carla::PythonUtil::AcquireGIL lock;
    try {
      py::call<void>(callback->ptr(), py::object(message));
    } catch (const py::error_already_set &) {
      PyErr_Print();
    }
  };
}

// 17个模块的源代码文件+1个RSS模块
#include "V2XData.cpp"
#include "Geom.cpp"
#include "Actor.cpp"
#include "Blueprint.cpp"
#include "Client.cpp"
#include "Control.cpp"
#include "Exception.cpp"
#include "Map.cpp"
#include "Sensor.cpp"
#include "SensorData.cpp"
#include "Snapshot.cpp"
#include "Weather.cpp"
#include "World.cpp"
#include "Commands.cpp"
#include "TrafficManager.cpp"
#include "LightManager.cpp"
#include "OSM2ODR.cpp"

#ifdef LIBCARLA_RSS_ENABLED
#include "AdRss.cpp"
#endif

BOOST_PYTHON_MODULE(libcarla) {
  using namespace boost::python;
#if PY_MAJOR_VERSION < 3 || PY_MINOR_VERSION < 7
  PyEval_InitThreads();
#endif
  scope().attr("__path__") = "libcarla";
  export_geom();
  export_control();
  export_blueprint();
  export_actor();
  export_sensor();
  export_sensor_data();
  export_snapshot();
  export_weather();
  export_world();
  export_map();
  export_client();
  export_exception();
  export_commands();
  export_trafficmanager();
  export_lightmanager();
  #ifdef LIBCARLA_RSS_ENABLED
  export_ad_rss();
  #endif
  export_osm2odr();
  // 这里的 export_xxx 函数应该是将相应模块中的类、函数等内容导出到 Python 中，使得它们可以在 Python 中使用。
  // 对于不同的模块，会根据其功能导出不同的接口，例如几何模块可能导出一些几何计算相关的函数，Actor 模块可能导出和角色相关的类和函数等。
  // 例如：export_actor 可能会导出与角色创建、角色属性操作、角色行为控制等相关的功能。
  // 而 export_control 可能会导出一些控制相关的功能，比如车辆的控制、物体的控制等。
  // 同时，通过这些 export 函数，C++ 代码可以通过 Boost.Python 库和 Python 代码进行交互。
  // 例如将 C++ 中的类和函数包装为 Python 对象，让 Python 代码可以调用。
  // 同时，通过上面的一些宏和模板函数，处理了参数传递、返回值处理、GIL 的获取和释放等问题，以确保在 C++ 和 Python 交互过程中的线程安全和数据类型转换。
  // 如 CALL_WITHOUT_GIL 等宏方便了在释放 GIL 的情况下调用函数，避免 GIL 带来的性能问题。
  // PythonLitstToVector 函数用于将 Python 列表转换为 C++ 的 vector 容器，方便 C++ 代码处理列表数据。
  // OptionalToPythonObject 函数用于将 C++ 中的可选类型转换为 Python 对象，处理可选值的情况。
  // MakeCallback 函数用于创建一个安全的回调函数，它会检查回调是否可调用，将回调存储在智能指针中，并在调用时处理异常和 GIL 的获取。
}
