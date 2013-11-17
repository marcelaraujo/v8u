/*
 * V8Utilities - Sugar for your Node C++ addons
 * Copyright (c) 2012, Xavier Mendez
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  - Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef V8U_HPP
#define	V8U_HPP

#include <string>
#include <exception>
#include <map>

#include <node.h>
#include <node_version.h>
#include <v8.h>

namespace v8u {

#if NODE_VERSION_AT_LEAST(0,11,8)
  #define V8_STHROW(VALUE) {args->GetIsolate()->ThrowException(VALUE); return;}
  #define V8_STHROW_NR V8_STHROW
  #define V8_RET(VALUE) {args->GetReturnValue()->Set(VALUE); return;}
  #define __v8_implicit_return(HANDLE)
#else
  #define V8_STHROW(VALUE) return v8::ThrowException(VALUE)
  #define V8_STHROW_NR(VALUE) {v8::ThrowException(VALUE); return;}
  #define V8_RET(VALUE) return scope.Close(VALUE)
  #define __v8_implicit_return(HANDLE) return HANDLE;
#endif

// V8 exception wrapping

#define V8_THROW(VALUE) throw v8::Persistent<v8::Value>::New(VALUE)

#define V8_WRAP_START()                                                        \
  v8::HandleScope scope;                                                       \
  try {

#define __v8_wrap_end(throw_type)                                              \
  } catch (v8::Persistent<v8::Value>& err) {                                   \
    err.Dispose();                                                             \
    throw_type(err);                                                           \
  } catch (std::exception& err) {                                              \
    throw_type(v8::Exception::Error(v8::String::New(err.what())));             \
  } catch (v8::Handle<v8::Value>& err) {                                       \
    throw_type(err);                                                           \
  } catch (v8::Value*& err) {                                                  \
    throw_type(v8::Handle<v8::Value>(err));                                    \
  } catch (std::string& err) {                                                 \
    throw_type(v8::Exception::Error(v8::String::New(err.data(), err.length())));\
  } catch (...) {                                                              \
    throw_type(v8::Exception::Error(v8::String::New("Unknown error!")));       \
  }

#define V8_WRAP_END()    __v8_wrap_end(V8_STHROW)
#define V8_WRAP_END_NR() __v8_wrap_end(V8_STHROW_NR)

// JS arguments

inline void CheckArguments(int min, const v8::Arguments& args) {
  if (args.Length() < min)
    V8_THROW(v8::Exception::RangeError(v8::String::New("Not enough arguments.")));
}

// V8 callback templates

#if NODE_VERSION_AT_LEAST(0,11,8)
  #define V8_SCB(IDENTIFIER)                                                   \
    void IDENTIFIER(const v8::FunctionCallbackInfo<v8::Value>& args)
#else
  #define V8_SCB(IDENTIFIER)                                                   \
    v8::Handle<v8::Value> IDENTIFIER(const v8::Arguments& args)
#endif

#define V8_CB(IDENTIFIER)                                                      \
V8_SCB(IDENTIFIER) {                                                           \
  V8_WRAP_START()

#define V8_CB_END()                                                            \
    __v8_implicit_return(v8::Undefined())                                      \
  V8_WRAP_END()                                                                \
}

// V8 getter templates

#if NODE_VERSION_AT_LEAST(0,11,8)
  #define __v8_getter(ID)                                                      \
    void ID(v8::Local<v8::String> name,                                        \
            const v8::PropertyCallbackInfo<Value>& info)
#else
  #define __v8_getter(ID)                                                      \
    v8::Handle<v8::Value> ID(v8::Local<v8::String> name,                       \
                             const v8::AccessorInfo& info)
#endif

#define V8_SGET(IDENTIFIER) static __v8_getter(IDENTIFIER)
#define V8_ESGET(TYPE, IDENTIFIER) __v8_getter(TYPE::IDENTIFIER)

#define V8_GET(IDENTIFIER)                                                     \
V8_SGET(IDENTIFIER) {                                                          \
  V8_WRAP_START()

#define V8_EGET(TYPE, IDENTIFIER)                                              \
V8_ESGET(TYPE, IDENTIFIER) {                                                   \
  V8_WRAP_START()

#define V8_GET_END()                                                           \
  V8_WRAP_END()                                                                \
}

// V8 setter templates

#if NODE_VERSION_AT_LEAST(0,11,8)
  #define __v8_setter(ID)                                                      \
    void ID(v8::Local<v8::String> name,                                        \
            v8::Local<v8::Value> value,                                        \
            const v8::PropertyCallbackInfo<Value>& info)
#else
  #define __v8_setter(ID)                                                      \
    void ID(v8::Local<v8::String> name,                                        \
            v8::Local<v8::Value> value,                                        \
            const v8::AccessorInfo& info)
#endif

#define V8_SSET(IDENTIFIER) static __v8_setter(IDENTIFIER)
#define V8_ESSET(TYPE, IDENTIFIER) __v8_setter(TYPE::IDENTIFIER)

#define V8_SET(IDENTIFIER)                                                     \
V8_SSET(IDENTIFIER) {                                                          \
  V8_WRAP_START()

#define V8_ESET(TYPE, IDENTIFIER)                                              \
V8_ESSET(TYPE, IDENTIFIER) {                                                   \
  V8_WRAP_START()

#define V8_SET_END()                                                           \
  V8_WRAP_END_NR()                                                             \
}


// Other class-specific templates

#define __v8_ctor {                                                            \
  if (args[0]->IsExternal()) return args.This();                               \
  if (!args.IsConstructCall())                                                 \
    V8_STHROW(v8u::ReferenceErr("You must call this as a constructor"));       \
  V8_WRAP_START()                                                              \
    v8::Local<v8::Object> hdl = args.This();

#define V8_SCTOR() static V8_SCB(NewInstance)
#define V8_ESCTOR(TYPE)   V8_SCB(TYPE::NewInstance)

#define V8_CTOR()      V8_SCTOR() __v8_ctor
#define V8_ECTOR(TYPE) V8_ESCTOR(TYPE) __v8_ctor

#define V8_CTOR_END()                                                          \
    __v8_implicit_return(hdl)                                                  \
  V8_WRAP_END()                                                                \
}

// Special constructors: use within V8_[E]SCTOR() ------------------------------
#define V8_CTOR_NO_ALL                                                         \
  V8_STHROW(v8u::TypeErr("No instances of this exact type may be constructed."));
#define V8_CTOR_NO_JS                                                          \
  if (args[0]->IsExternal()) return args.This();                               \
  V8_STHROW(v8u::TypeErr("You can't construct instances of this type directly."));
//------------------------------------------------------------------------------

//// For use with V8_CTOR only!
#define V8_WRAP(INSTANCE) (INSTANCE)->Wrap(hdl)

#define V8_M_UNWRAP(CPP_TYPE, OBJ)                                             \
  if (!CPP_TYPE::_templ->HasInstance(OBJ))                                     \
    V8_STHROW(v8u::TypeErr("Invalid object unwrapped."));                      \
  CPP_TYPE* inst = node::ObjectWrap::Unwrap<CPP_TYPE>(OBJ);

#if NODE_VERSION_AT_LEAST(0,11,4)
  #define __node_get_handle() handle()
#else
  #define __node_get_handle() handle_
#endif

#define V8_STYPE(CPP_TYPE)                                                     \
  static v8::FunctionTemplate* _templ;                                         \
  /**
   * Returns the unique V8 v8::Object corresponding to this C++ instance.
   * For this to work, you should use V8_CL_CTOR.
   *
   * CALLING Wrapped() WITHIN A CONSTRUCTOR MAY YIELD UNEXPECTED RESULTS,
   * EVENTUALLY MAKING YOU BASH YOUR HEAD AGAINST A WALL. YOU HAVE BEEN WARNED.
   **/                                                                         \
  virtual v8::Local<v8::Object> Wrapped();                                     \
  static bool HasInstance(v8::Handle<v8::Object> obj);                         \
  inline static CPP_TYPE* Unwrap(v8::Handle<v8::Object> obj)

#define V8_TYPE(CPP_TYPE)                                                      \
  static v8::FunctionTemplate* _templ;                                         \
  /**
   * Returns the unique V8 v8::Object corresponding to this C++ instance.
   * For this to work, you should use V8_[E]CTOR.
   * V8_[E]SCTOR with V8_CTOR_NO_JS will work as well.
   *
   * CALLING Wrapped() WITHIN A CONSTRUCTOR MAY YIELD UNEXPECTED RESULTS,
   * EVENTUALLY MAKING YOU BASH YOUR HEAD AGAINST A WALL. YOU HAVE BEEN WARNED.
   **/                                                                         \
  virtual v8::Local<v8::Object> Wrapped() {                                    \
    v8::HandleScope scope;                                                     \
                                                                               \
    v8::Handle<v8::Object> handle = __node_get_handle();                       \
    if (handle.IsEmpty()) {                                                    \
      v8::Handle<v8::Value> args [1] = {v8::External::New(this)};              \
      handle = _templ->GetFunction()->NewInstance(1,args);                     \
      Wrap(handle);                                                            \
    }                                                                          \
    return scope.Close(handle);                                                \
  }                                                                            \
  static bool HasInstance(v8::Handle<v8::Object> obj) {                        \
    return _templ->HasInstance(obj);                                           \
  }                                                                            \
  inline static CPP_TYPE* Unwrap(v8::Handle<v8::Object> obj) {                 \
    if (_templ->HasInstance(obj)) return node::ObjectWrap::Unwrap<CPP_TYPE>(obj);\
    V8_THROW(v8::Exception::TypeError(v8::String::New("Invalid object unwrapped.")));\
  }

#define V8_ETYPE(TYPE)                                                         \
  v8::Local<v8::Object> TYPE::Wrapped() {                                      \
    v8::HandleScope scope;                                                     \
                                                                               \
    v8::Handle<v8::Object> handle = __node_get_handle();                       \
    if (handle.IsEmpty()) {                                                    \
      v8::Handle<v8::Value> args [1] = {v8::External::New(this)};              \
      handle = _templ->GetFunction()->NewInstance(1,args);                     \
      Wrap(handle);                                                            \
    }                                                                          \
    return scope.Close(handle);                                                \
  }                                                                            \
  bool TYPE::HasInstance(v8::Handle<v8::Object> obj) {                         \
    return _templ->HasInstance(obj);                                           \
  }                                                                            \
  TYPE* TYPE::Unwrap(v8::Handle<v8::Object> obj) {                             \
    if (_templ->HasInstance(obj)) return node::ObjectWrap::Unwrap<TYPE>(obj);  \
    V8_THROW(v8u::TypeErr("Invalid object unwrapped."));                       \
  }

#define V8_POST_TYPE(CPP_TYPE) v8::FunctionTemplate* CPP_TYPE::_templ = NULL;

// Dealing with V8 persistent handles

template <class T> inline void ClearPersistent(v8::Persistent<T>& handle) {
  if (handle.IsEmpty()) return;
  handle.Dispose();
  handle.Clear();
}

template <class T> inline void SetPersistent(v8::Persistent<T>& handle, v8::Handle<T> value) {
  ClearPersistent<T>(handle);
  if (value.IsEmpty()) return;
  handle = v8::Persistent<T>::New(value);
}

template <class T> inline v8::Persistent<T> Persist(v8::Handle<T> handle) {
  return v8::Persistent<T>::New(handle);
}

template <class T> class Persisted {
public:
  inline Persisted() {};
  inline Persisted(v8::Handle<T> value) : handle(v8::Persistent<T>::New(value)) {}
  inline ~Persisted() {
    if (!handle.IsEmpty()) handle.Dispose();
  }
  inline Persisted(Persisted<T>& other) : handle(v8::Persistent<T>::New(other.handle)) {}
  inline v8::Persistent<T> operator*() const {
    return handle;
  }
  inline Persisted<T>& operator=(const Persisted<T>& other) {
    if (&other == this) return *this;
    SetPersistent<T>(handle, other.handle);
    return *this;
  }
  inline bool operator==(const Persisted<T>& other) const {
    return handle==other.handle;
  }
  inline bool IsEmpty() const {
    return handle.IsEmpty();
  }
  inline void Clear() {
    ClearPersistent<T>(handle);
  }
  inline T* operator->() const {
    return *handle;
  }
private:
  v8::Persistent<T> handle;
};

// Type shortcuts

inline v8::Local<v8::Integer> Int(int64_t integer) {
  return v8::Integer::New(integer);
}

inline v8::Local<v8::Integer> Uint(uint32_t integer) {
  return v8::Integer::NewFromUnsigned(integer);
}

inline v8::Local<v8::String> Str(const char* data, int length = -1) {
  return v8::String::New(data, length);
}

inline v8::Local<v8::String> Str(std::string str) {
  return v8::String::New(str.data(), str.length());
}

inline v8::Local<v8::String> Symbol(const char* data, int length = -1) {
  return v8::String::NewSymbol(data, length);
}

inline v8::Local<v8::Object> Obj() {
  return v8::Object::New();
}

inline v8::Local<v8::Array> Arr(int length = 0) {
  return v8::Array::New(length);
}

#define __v8_error_ctor(ERROR)                                                 \
inline v8::Local<v8::Value> ERROR##Err(const char* msg) {                      \
  return v8::Exception::ERROR##Error(v8::String::New(msg));                    \
}

__v8_error_ctor()
__v8_error_ctor(Range)
__v8_error_ctor(Reference)
__v8_error_ctor(Syntax)
__v8_error_ctor(Type)

inline v8::Local<v8::Number> Num(double number) {
  return v8::Number::New(number);
}

inline v8::Handle<v8::Boolean> Bool(bool boolean) {
  return v8::Boolean::New(boolean);
}

inline v8::Local<v8::Function> Func(v8::InvocationCallback function) {
  return v8::FunctionTemplate::New(function)->GetFunction();
}

inline v8::Local<v8::FunctionTemplate> Template(v8::InvocationCallback function) {
  return v8::FunctionTemplate::New(function);
}

// Type casting/unwraping shortcuts

inline double Num(v8::Handle<v8::Value> hdl) {
  return hdl->NumberValue();
}

inline int32_t Int(v8::Handle<v8::Value> hdl) {
  return hdl->Int32Value();
}

inline uint32_t Uint(v8::Handle<v8::Value> hdl) {
  return hdl->Uint32Value();
}

inline v8::Handle<v8::Object> Obj(v8::Handle<v8::Value> hdl) {
  return v8::Handle<v8::Object>::Cast(hdl);
}
inline v8::Local<v8::Object> Obj(v8::Local<v8::Value> hdl) {
  return v8::Local<v8::Object>::Cast(hdl);
}
inline v8::Persistent<v8::Object> Obj(v8::Persistent<v8::Value> hdl) {
  return v8::Persistent<v8::Object>::Cast(hdl);
}

inline v8::Handle<v8::Array> Arr(v8::Handle<v8::Value> hdl) {
  return v8::Handle<v8::Array>::Cast(hdl);
}
inline v8::Local<v8::Array> Arr(v8::Local<v8::Value> hdl) {
  return v8::Local<v8::Array>::Cast(hdl);
}
inline v8::Persistent<v8::Array> Arr(v8::Persistent<v8::Value> hdl) {
  return v8::Persistent<v8::Array>::Cast(hdl);
}

inline v8::Handle<v8::Function> Func(v8::Handle<v8::Value> hdl) {
  return v8::Handle<v8::Function>::Cast(hdl);
}
inline v8::Local<v8::Function> Func(v8::Local<v8::Value> hdl) {
  return v8::Local<v8::Function>::Cast(hdl);
}
inline v8::Persistent<v8::Function> Func(v8::Persistent<v8::Value> hdl) {
  return v8::Persistent<v8::Function>::Cast(hdl);
}

template <class T> inline v8::Handle<T> Cast(v8::Handle<v8::Value> hdl) {
  return v8::Handle<T>::Cast(hdl);
}
template <class T> inline v8::Local<T> Cast(v8::Local<v8::Value> hdl) {
  return v8::Local<T>::Cast(hdl);
}
template <class T> inline v8::Persistent<T> Cast(v8::Persistent<v8::Value> hdl) {
  return v8::Persistent<T>::Cast(hdl);
}

inline bool Bool(v8::Handle<v8::Value> hdl) {
  return hdl->BooleanValue();
}

// Defining things

#define V8_DEF_TYPE_PRE()                                                      \
  v8::Persistent<v8::FunctionTemplate> templ;                                  \
  v8::Local<v8::ObjectTemplate> prot;                                          \
  v8::Local<v8::ObjectTemplate> inst;                                          \
  v8::Handle<v8::String> __cname;

#define V8_DEF_TYPE(V8_NAME)                                                   \
  templ = v8::Persistent<v8::FunctionTemplate>::New(                           \
      v8::FunctionTemplate::New(NewInstance));                                 \
  __cname = v8::String::NewSymbol(V8_NAME);                                    \
  templ->SetClassName(__cname);                                                \
  inst = templ->InstanceTemplate();                                            \
  inst->SetInternalFieldCount(1);                                              \
  prot = templ->PrototypeTemplate();

#define V8_DEF_ACC(V8_NAME, GETTER, SETTER)                                    \
  inst->SetAccessor(v8::String::NewSymbol(V8_NAME), GETTER, SETTER)

#define V8_DEF_GET(V8_NAME, GETTER)                                            \
  inst->SetAccessor(v8::String::NewSymbol(V8_NAME), GETTER)

//FIXME: add V8_DEF_SET

#define V8_DEF_CB(V8_NAME, CPP_METHOD)                                         \
  inst->Set(v8u::Symbol(V8_NAME), v8u::Func(CPP_METHOD))

#define V8_INHERIT(CPP_TYPE) templ->Inherit(CPP_TYPE::_templ)

// Templates for definition methods on Node

//// Generic define function

#define NODE_DEF(IDENTIFIER)                                                   \
  void IDENTIFIER(v8::Handle<v8::Object> target)

//// Module define function

#define NODE_DEF_MAIN()                                                        \
  extern "C" {                                                                 \
    NODE_DEF(init) {                                                           \
      v8::HandleScope scope;

#define NODE_DEF_MAIN_END(MODULE) }                                            \
    NODE_MODULE(MODULE, init); }

//// Type (class) define function

#define NODE_SDEF_TYPE() static NODE_DEF(init)
#define NODE_ESDEF_TYPE(TYPE)   NODE_DEF(TYPE::init)

#define NODE_DEF_TYPE(V8_NAME)                                                 \
  inline NODE_SDEF_TYPE() {                                                    \
    v8::HandleScope scope;                                                     \
    V8_DEF_TYPE_PRE()                                                          \
    V8_DEF_TYPE(V8_NAME)

#define NODE_EDEF_TYPE(TYPE, V8_NAME)                                          \
  NODE_ESDEF_TYPE(TYPE) {                                                      \
    v8::HandleScope scope;                                                     \
    V8_DEF_TYPE_PRE()                                                          \
    V8_DEF_TYPE(V8_NAME)

#define NODE_DEF_TYPE_END()                                                    \
    target->Set(__cname, templ->GetFunction());                                \
  }

//// V8_TYPE + NODE_DEF_TYPE = NODE_TYPE

#define NODE_STYPE(CPP_TYPE)                                                   \
  V8_STYPE(CPP_TYPE);                                                          \
  NODE_SDEF_TYPE()

#define NODE_TYPE(CPP_TYPE, V8_NAME)                                           \
  V8_TYPE(CPP_TYPE)                                                            \
  inline NODE_SDEF_TYPE() {                                                    \
    if (_templ) {                                                              \
      target->Set(v8::String::NewSymbol(V8_NAME), v8::Handle<v8::Function>(_templ->GetFunction()));\
      return;                                                                  \
    }                                                                          \
    v8::HandleScope scope;                                                     \
    V8_DEF_TYPE_PRE()                                                          \
    V8_DEF_TYPE(V8_NAME)

#define NODE_ETYPE(TYPE, V8_NAME)                                              \
  V8_ETYPE(TYPE)                                                               \
  NODE_ESDEF_TYPE(TYPE) {                                                      \
    if (_templ) {                                                              \
      target->Set(v8::String::NewSymbol(V8_NAME), v8::Handle<v8::Function>(_templ->GetFunction()));\
      return;                                                                  \
    }                                                                          \
    v8::HandleScope scope;                                                     \
    V8_DEF_TYPE_PRE()                                                          \
    V8_DEF_TYPE(V8_NAME)

#define NODE_TYPE_END()                                                        \
    _templ = *templ;                                                           \
  NODE_DEF_TYPE_END()

};

#endif	/* V8U_HPP */
