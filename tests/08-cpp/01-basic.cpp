#include <functional>
#include <iostream>

#include "../ctest.h"

#include <bindings/cpp/onion.hpp>
#include <bindings/cpp/response.hpp>
#include <bindings/cpp/url.hpp>
#include <bindings/cpp/request.hpp>

class DefaultHandler : public Onion::Handler{
  onion_connection_status operator()(Onion::Request &req, Onion::Response &res){
    res<<std::endl<<"Mundo "<<std::hex<<255<<"\n";
    return OCS_PROCESSED;
  }
};

onion_connection_status handler(Onion::Request &req, Onion::Response &res){
  res<<"Hola mundo desde el handler."<<std::endl;
  return OCS_PROCESSED;
}

class MyHandler{
  int n;
public:
  MyHandler(){ n=rand(); }
  onion_connection_status index(Onion::Request &req, Onion::Response &res){
    res<<"index "<<n<<std::endl;
		return OCS_PROCESSED;
  }
  onion_connection_status error(Onion::Request &req, Onion::Response &res){
    res<<"error "<<n<<std::endl;
		return OCS_PROCESSED;
  }
};

void t01_basic(){
  INIT_LOCAL()
  
  Onion::Onion o;
  MyHandler m;

  Onion::Url url(o);
  
  url.add("", handler);
  url.add("static", "Estatico");
  url.add<MyHandler>("m", &m, &MyHandler::index);
  
  o.setInternalErrorHandler( new Onion::HandlerMethod<MyHandler>(&m, &MyHandler::error) );
  
  o.listen();
  
  END_LOCAL()
}

int main(int argc, char **argv){
  START();
  t01_basic();

  
  END();
}
