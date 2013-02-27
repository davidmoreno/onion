#ifndef __URL_HPP__
#define __URL_HPP__

#include "onion.hpp"
#include "handler.hpp"
#include <onion/url.h>
#include <onion/onion.h>

namespace Onion{
  class Url{
    onion_url *ptr;
  public:
    Url(onion_url *_ptr) : ptr(_ptr){};
    Url(Onion *o) {
      ptr=onion_root_url(o->c_handler());
    }
    Url(Onion &o) {
      ptr=onion_root_url(o.c_handler());
    }

    onion_url *c_handler(){ return ptr;  }
    
    bool add(const std::string &url, Handler *h){
      return onion_url_add_handler(ptr,url.c_str(),h->c_handler());
    }
    bool add(const std::string &url, HandlerFunction::fn_t fn){
      return add(url,new HandlerFunction(fn));
    }
    template<class T>
    bool add(const std::string &url, T *o, onion_connection_status (T::*fn)(Request &,Response &)){
      return add(url,new HandlerMethod<T>(o,fn));
    }
    bool add(const std::string &url, const std::string &s, int http_code=200){
      onion_url_add_static(ptr,url.c_str(),s.c_str(),http_code);
    }
  };
}

#endif
