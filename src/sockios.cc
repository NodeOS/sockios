#include <iostream>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/route.h>
#include <v8.h>
#include <node.h>
#include <unistd.h>
#include <cstring>

// http://man7.org/linux/man-pages/man7/netdevice.7.html
// http://linux.about.com/library/cmd/blcmdl7_netdevice.htm

using namespace std;
using namespace v8;
using namespace node;

namespace NodeOS
{
  
  static Handle<Object> GetFlags(){
    HandleScope scope;
    Local<Object> flags = Object::New();
    
    flags->Set( String::New("SIOCGIFNAME"), Integer::New(SIOCGIFNAME) );
    flags->Set( String::New("SIOCGIFINDEX"), Integer::New(SIOCGIFINDEX) );
    flags->Set( String::New("SIOCGIFFLAGS"), Integer::New(SIOCGIFFLAGS) );
    flags->Set( String::New("SIOCSIFFLAGS"), Integer::New(SIOCSIFFLAGS) );
    flags->Set( String::New("SIOCGIFPFLAGS"), Integer::New(SIOCGIFPFLAGS) );
    flags->Set( String::New("SIOCSIFPFLAGS"), Integer::New(SIOCSIFPFLAGS) );
    flags->Set( String::New("SIOCGIFADDR"), Integer::New(SIOCGIFADDR) );
    flags->Set( String::New("SIOCSIFADDR"), Integer::New(SIOCSIFADDR) );
    flags->Set( String::New("SIOCGIFDSTADDR"), Integer::New(SIOCGIFDSTADDR) );
    flags->Set( String::New("SIOCSIFDSTADDR"), Integer::New(SIOCSIFDSTADDR) );
    flags->Set( String::New("SIOCGIFBRDADDR"), Integer::New(SIOCGIFBRDADDR) );
    flags->Set( String::New("SIOCSIFBRDADDR"), Integer::New(SIOCSIFBRDADDR) );
    flags->Set( String::New("SIOCGIFNETMASK"), Integer::New(SIOCGIFNETMASK) );
    flags->Set( String::New("SIOCSIFNETMASK"), Integer::New(SIOCSIFNETMASK) );
    flags->Set( String::New("SIOCGIFMETRIC"), Integer::New(SIOCGIFMETRIC) );
    flags->Set( String::New("SIOCSIFMETRIC"), Integer::New(SIOCSIFMETRIC) );
    flags->Set( String::New("SIOCGIFMTU"), Integer::New(SIOCGIFMTU) );
    flags->Set( String::New("SIOCSIFMTU"), Integer::New(SIOCSIFMTU) );
    flags->Set( String::New("SIOCGIFHWADDR"), Integer::New(SIOCGIFHWADDR) );
    flags->Set( String::New("SIOCSIFHWADDR"), Integer::New(SIOCSIFHWADDR) );
    flags->Set( String::New("SIOCSIFHWBROADCAST"), Integer::New(SIOCSIFHWBROADCAST) );
    flags->Set( String::New("SIOCGIFMAP"), Integer::New(SIOCGIFMAP) );
    flags->Set( String::New("SIOCSIFMAP"), Integer::New(SIOCSIFMAP) );
    flags->Set( String::New("SIOCADDMULTI"), Integer::New(SIOCADDMULTI) );
    flags->Set( String::New("SIOCGIFTXQLEN"), Integer::New(SIOCGIFTXQLEN) );
    flags->Set( String::New("SIOCSIFTXQLEN"), Integer::New(SIOCSIFTXQLEN) );
    flags->Set( String::New("SIOCSIFNAME"), Integer::New(SIOCSIFNAME) );
    flags->Set( String::New("SIOCGIFCONF"), Integer::New(SIOCGIFCONF) );
    
    return scope.Close(flags);
  }
  
  // re-usable fd for ioctl on ipv4 sockets
  // this does not refer to a particular network device,
  // rather it identifies that ioctl should apply operations
  // to ipv4-specific network configurations
  const int inet_sock_fd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
  
  // hard-coded loopack name
  const char *loopbackName = "lo";
  
  // hex-format for hardaware addresses
  const char *ether_fmt = "%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X";
  
  const int IFREQ_SIZE = sizeof( ifreq );
  
  static Handle<Value> GetHardwareAddress(const Arguments& args){
    HandleScope scope;
    ifreq req;
    char out [18];
    
    String::Utf8Value name(args[0]);
    
    strcpy( req.ifr_name, *name );
    
    ioctl(inet_sock_fd, SIOCGIFHWADDR, &req);
    
    char* hwaddr = req.ifr_hwaddr.sa_data;
    
    sprintf(out,ether_fmt,hwaddr[0],hwaddr[1],hwaddr[2],hwaddr[3],hwaddr[4],hwaddr[5]);
    
    return scope.Close(String::New(out));
  }
  
  Handle<Object> SockaddrInToObject(sockaddr* sock){
    HandleScope scope;
    
    sockaddr_in *sock_in = (struct sockaddr_in*) sock;
    
    Handle<Object> sock_obj = Object::New();
    
    const char* family = "AF_INET";
    uint32_t addr = sock_in->sin_addr.s_addr;
    uint32_t port = sock_in->sin_port;
    
    sock_obj->Set( String::NewSymbol("sin_family"), String::New(family) );
    sock_obj->Set( String::NewSymbol("sin_port"), Integer::New(port) );
    sock_obj->Set( String::NewSymbol("sin_addr"), Integer::New(addr) );
    
    return scope.Close(sock_obj);
  }
  
  Handle<Object> IFReqToObject(ifreq *ifreq, Handle<Object> ifreq_obj ){
    HandleScope scope;
    
    Handle<Object> sockaddr = SockaddrInToObject( &ifreq->ifr_addr );
    
    ifreq_obj->Set( String::NewSymbol("sockaddr"), sockaddr );
    
    return scope.Close(ifreq_obj);
  }
  
  static Handle<Value> Ioctl(const Arguments& args){
    HandleScope scope;
    
    int sfd = Handle<Number>::Cast(args[0])->Value();
    int req = Handle<Number>::Cast(args[1])->Value();
    
    ifreq ifreq;
    
    int res = ioctl( sfd, req, &ifreq );
    
    return scope.Close(Integer::New(res));
  }
  
  // format and return IP address as dotted decimal
  // from a sockaddr object
  char* FormatIP(sockaddr *sock_addr){
    return inet_ntoa(((struct sockaddr_in*) sock_addr)->sin_addr);
  }
  
  static Handle<Value> GetNetmasks(const Arguments& args){
    HandleScope scope;
    ifreq req;
    
    String::Utf8Value name(args[0]);
    
    strcpy( req.ifr_name, *name );
    
    ioctl(inet_sock_fd, SIOCGIFNETMASK, &req);
    
    char* addr = FormatIP(&req.ifr_netmask);
    
    return scope.Close(String::New(addr));
  }
  
  static Handle<Value> GetAddresses(const Arguments& args){
    
    HandleScope scope;
    
    int buff_count = 10;
    int IFREQ_SIZE = sizeof( ifreq );
    
    ifconf ifaces;
    ifaces.ifc_req = new ifreq[buff_count];
    ifaces.ifc_len = buff_count * IFREQ_SIZE;
    
    ioctl(inet_sock_fd,SIOCGIFCONF,&ifaces);
    int len = ifaces.ifc_len / IFREQ_SIZE;
    
    Handle<Object> obj = Object::New();
    
    for(int i=0; i<len; i++){
      
      ifreq req = ifaces.ifc_req[i];
      
      struct sockaddr *saddr = &req.ifr_addr;
      
      char* addr = FormatIP(saddr);
      
      Handle<Object> iface = Object::New();
      
      char* name = req.ifr_name;
      
      iface->Set(String::New("address"), String::New(addr));
      
      obj->Set(String::New(name), iface);
      
    }
    
    // free memory
    delete ifaces.ifc_req;
    
    return scope.Close(obj);
      
  }
  
  // Non-V8 Function
  int setAddress(char* iface, char* ip){
    
    struct ifreq ifr;
    struct sockaddr_in sin;
    
    // https://groups.google.com/forum/#!topic/nodejs/aNeC6kyZcFI
    
    // set the interface
    strncpy(ifr.ifr_name, iface, IFNAMSIZ);
    
    // Assign IP address
    in_addr_t in_addr = inet_addr(ip);
    sin.sin_addr.s_addr = in_addr;
    
    // I don't think the port matters
    // we just need a fd that refers to a inet socket
    sin.sin_family = AF_INET;
    sin.sin_port = 0;
    
    memcpy(&ifr.ifr_addr, &sin, sizeof(struct sockaddr));
    
    // Request to set IP address to interface
    return ioctl(inet_sock_fd, SIOCSIFADDR, &ifr); 
    
  }
  
  static Handle<Value> SetAddress(const Arguments& args) {
    HandleScope scope;
    
    String::Utf8Value iface(args[0]);
    String::Utf8Value ip(args[1]);
    
    int res = setAddress(*iface, *ip);
    
    return scope.Close(Integer::New(res));
  }
  
  
  static Handle<Value> LoopbackUp(const Arguments& args) {
    HandleScope scope;
    
    struct ifreq ifr;
    int res = 0;
    ifr.ifr_flags = IFF_UP|IFF_LOOPBACK|IFF_RUNNING;
    strncpy(ifr.ifr_name,loopbackName,IFNAMSIZ);
    
    res = ioctl(inet_sock_fd,SIOCSIFFLAGS,&ifr);
    
    return scope.Close(Integer::New(res));
  }
  
  int ifUp(char* iface){
    struct ifreq ifr;
    
    ifr.ifr_flags = IFF_UP|IFF_LOOPBACK|IFF_RUNNING;
    strncpy(ifr.ifr_name,iface,IFNAMSIZ);
    
    return ioctl(inet_sock_fd,SIOCSIFFLAGS,&ifr);
  }
  
  //http://lxr.free-electrons.com/source/Documentation/networking/ifenslave.c#L990
  static Handle<Value> IfUp(const Arguments& args) {
    HandleScope scope;
    
    String::Utf8Value iface(args[0]);
    
    int res = ifUp(*iface);
    
    return scope.Close(Integer::New(res));
  }
  
  int setDefGateway(const char * defGateway)
  {
    struct rtentry rm;
    struct sockaddr_in ic_gateway ;// Gateway IP address
    int err;

    memset(&rm, 0, sizeof(rm));
    
    ic_gateway.sin_family = AF_INET;
    ic_gateway.sin_addr.s_addr = inet_addr(defGateway);
    ic_gateway.sin_port = 0;
    
    (( struct sockaddr_in*)&rm.rt_dst)->sin_family = AF_INET;
    (( struct sockaddr_in*)&rm.rt_dst)->sin_addr.s_addr = 0;
    (( struct sockaddr_in*)&rm.rt_dst)->sin_port = 0;

    (( struct sockaddr_in*)&rm.rt_genmask)->sin_family = AF_INET;
    (( struct sockaddr_in*)&rm.rt_genmask)->sin_addr.s_addr = 0;
    (( struct sockaddr_in*)&rm.rt_genmask)->sin_port = 0;

    memcpy((void *) &rm.rt_gateway, &ic_gateway, sizeof(ic_gateway));
    rm.rt_flags = RTF_UP | RTF_GATEWAY;
    if ((err = ioctl(inet_sock_fd, SIOCADDRT, &rm)) < 0)
    {
      printf("SIOCADDRT failed , ret->%d\n",err);
      return -1;
    }
    return 0;
  }
  
  // http://pic.dhe.ibm.com/infocenter/aix/v7r1/index.jsp?topic=%2Fcom.ibm.aix.commtechref%2Fdoc%2Fcommtrf2%2Fioctl_socket_control_operations.htm
  static Handle<Value> SetDefaultGateway(const Arguments& args) {
    HandleScope scope;
    
    String::Utf8Value ip(args[0]);
    
    int res = setDefGateway(*ip);
    
    return scope.Close(Integer::New(res));
  }
  
  static void init(Handle<Object> target) {
    
    target->Set(String::NewSymbol("flags"), GetFlags() );
    
    target->Set(String::NewSymbol("ioctl"),
      FunctionTemplate::New(Ioctl)->GetFunction());
    
    target->Set(String::NewSymbol("getHardwareAddress"),
      FunctionTemplate::New(GetHardwareAddress)->GetFunction());
    target->Set(String::NewSymbol("getNetmasks"),
      FunctionTemplate::New(GetNetmasks)->GetFunction());
    target->Set(String::NewSymbol("getAddresses"),
      FunctionTemplate::New(GetAddresses)->GetFunction());
    target->Set(String::NewSymbol("setDefaultGateway"),
      FunctionTemplate::New(SetDefaultGateway)->GetFunction());
    target->Set(String::NewSymbol("setAddress"),
      FunctionTemplate::New(SetAddress)->GetFunction());
    target->Set(String::NewSymbol("ifup"),
      FunctionTemplate::New(IfUp)->GetFunction());
    target->Set(String::NewSymbol("loopbackUp"),
      FunctionTemplate::New(LoopbackUp)->GetFunction());
  }
}

NODE_MODULE(binding, NodeOS::init)
