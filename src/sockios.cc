#include <iostream>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/route.h>
#include <v8.h>
#include <node.h>
#include <unistd.h>
#include <cstring>
#include <nan.h>

// http://man7.org/linux/man-pages/man7/netdevice.7.html
// http://linux.about.com/library/cmd/blcmdl7_netdevice.htm

using namespace std;
using namespace v8;
using namespace node;

NAN_METHOD(GetFlags) {
  NanScope();
  Local<Object> flags = NanNew<Object>();

  flags->Set( NanNew<String>("SIOCGIFNAME"), NanNew<Integer>(SIOCGIFNAME) );
  flags->Set( NanNew<String>("SIOCGIFINDEX"), NanNew<Integer>(SIOCGIFINDEX) );
  flags->Set( NanNew<String>("SIOCGIFFLAGS"), NanNew<Integer>(SIOCGIFFLAGS) );
  flags->Set( NanNew<String>("SIOCSIFFLAGS"), NanNew<Integer>(SIOCSIFFLAGS) );
  flags->Set( NanNew<String>("SIOCGIFPFLAGS"), NanNew<Integer>(SIOCGIFPFLAGS) );
  flags->Set( NanNew<String>("SIOCSIFPFLAGS"), NanNew<Integer>(SIOCSIFPFLAGS) );
  flags->Set( NanNew<String>("SIOCGIFADDR"), NanNew<Integer>(SIOCGIFADDR) );
  flags->Set( NanNew<String>("SIOCSIFADDR"), NanNew<Integer>(SIOCSIFADDR) );
  flags->Set( NanNew<String>("SIOCGIFDSTADDR"), NanNew<Integer>(SIOCGIFDSTADDR) );
  flags->Set( NanNew<String>("SIOCSIFDSTADDR"), NanNew<Integer>(SIOCSIFDSTADDR) );
  flags->Set( NanNew<String>("SIOCGIFBRDADDR"), NanNew<Integer>(SIOCGIFBRDADDR) );
  flags->Set( NanNew<String>("SIOCSIFBRDADDR"), NanNew<Integer>(SIOCSIFBRDADDR) );
  flags->Set( NanNew<String>("SIOCGIFNETMASK"), NanNew<Integer>(SIOCGIFNETMASK) );
  flags->Set( NanNew<String>("SIOCSIFNETMASK"), NanNew<Integer>(SIOCSIFNETMASK) );
  flags->Set( NanNew<String>("SIOCGIFMETRIC"), NanNew<Integer>(SIOCGIFMETRIC) );
  flags->Set( NanNew<String>("SIOCSIFMETRIC"), NanNew<Integer>(SIOCSIFMETRIC) );
  flags->Set( NanNew<String>("SIOCGIFMTU"), NanNew<Integer>(SIOCGIFMTU) );
  flags->Set( NanNew<String>("SIOCSIFMTU"), NanNew<Integer>(SIOCSIFMTU) );
  flags->Set( NanNew<String>("SIOCGIFHWADDR"), NanNew<Integer>(SIOCGIFHWADDR) );
  flags->Set( NanNew<String>("SIOCSIFHWADDR"), NanNew<Integer>(SIOCSIFHWADDR) );
  flags->Set( NanNew<String>("SIOCSIFHWBROADCAST"), NanNew<Integer>(SIOCSIFHWBROADCAST) );
  flags->Set( NanNew<String>("SIOCGIFMAP"), NanNew<Integer>(SIOCGIFMAP) );
  flags->Set( NanNew<String>("SIOCSIFMAP"), NanNew<Integer>(SIOCSIFMAP) );
  flags->Set( NanNew<String>("SIOCADDMULTI"), NanNew<Integer>(SIOCADDMULTI) );
  flags->Set( NanNew<String>("SIOCGIFTXQLEN"), NanNew<Integer>(SIOCGIFTXQLEN) );
  flags->Set( NanNew<String>("SIOCSIFTXQLEN"), NanNew<Integer>(SIOCSIFTXQLEN) );
  flags->Set( NanNew<String>("SIOCSIFNAME"), NanNew<Integer>(SIOCSIFNAME) );
  flags->Set( NanNew<String>("SIOCGIFCONF"), NanNew<Integer>(SIOCGIFCONF) );

  NanReturnValue(flags);
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

NAN_METHOD(GetHardwareAddress) {
  NanScope();
  ifreq req;
  char out [18];

  String::Utf8Value name(args[0]);

  strcpy( req.ifr_name, *name );

  ioctl(inet_sock_fd, SIOCGIFHWADDR, &req);

  char* hwaddr = req.ifr_hwaddr.sa_data;

  sprintf(out,ether_fmt,hwaddr[0],hwaddr[1],hwaddr[2],hwaddr[3],hwaddr[4],hwaddr[5]);

  NanReturnValue(NanNew<String>(out));
}

NAN_METHOD(Ioctl) {
  NanScope();

  int sfd = Handle<Number>::Cast(args[0])->Value();
  int req = Handle<Number>::Cast(args[1])->Value();

  ifreq ifreq;

  int res = ioctl( sfd, req, &ifreq );

  NanReturnValue(NanNew<Integer>(res));
}

// format and return IP address as dotted decimal
// from a sockaddr object
char* FormatIP(sockaddr *sock_addr){
  return inet_ntoa(((struct sockaddr_in*) sock_addr)->sin_addr);
}

NAN_METHOD(GetNetmasks) {
  NanScope();
  ifreq req;

  String::Utf8Value name(args[0]);
  strcpy( req.ifr_name, *name );
  ioctl(inet_sock_fd, SIOCGIFNETMASK, &req);
  char* addr = FormatIP(&req.ifr_netmask);

  NanReturnValue(NanNew<String>(addr));
}

NAN_METHOD(GetAddresses) {
  NanScope();

  int buff_count = 10;
  int IFREQ_SIZE = sizeof( ifreq );

  ifconf ifaces;
  ifaces.ifc_req = new ifreq[buff_count];
  ifaces.ifc_len = buff_count * IFREQ_SIZE;

  ioctl(inet_sock_fd,SIOCGIFCONF,&ifaces);
  int len = ifaces.ifc_len / IFREQ_SIZE;

  Handle<Object> obj = NanNew<Object>();

  for(int i=0; i<len; i++){
    ifreq req = ifaces.ifc_req[i];

    struct sockaddr *saddr = &req.ifr_addr;

    char* addr = FormatIP(saddr);

    Handle<Object> iface = NanNew<Object>();

    char* name = req.ifr_name;

    iface->Set(NanNew<String>("address"), NanNew<String>(addr));

    obj->Set(NanNew<String>(name), iface);
  }

  // free memory
  delete ifaces.ifc_req;
  NanReturnValue(obj);
}

// Non-V8 Function
int setAddress(char* iface, char* ip) {
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

NAN_METHOD(SetAddress) {
  NanScope();

  String::Utf8Value iface(args[0]);
  String::Utf8Value ip(args[1]);

  int res = setAddress(*iface, *ip);

  NanReturnValue(NanNew<Integer>(res));
}


NAN_METHOD(LoopbackUp) {
  NanScope();

  struct ifreq ifr;
  int res = 0;
  ifr.ifr_flags = IFF_UP|IFF_LOOPBACK|IFF_RUNNING;
  strncpy(ifr.ifr_name,loopbackName,IFNAMSIZ);

  res = ioctl(inet_sock_fd,SIOCSIFFLAGS,&ifr);

  NanReturnValue(NanNew<Integer>(res));
}

int ifUp(char* iface) {
  struct ifreq ifr;

  ifr.ifr_flags = IFF_UP|IFF_LOOPBACK|IFF_RUNNING;
  strncpy(ifr.ifr_name,iface,IFNAMSIZ);

  return ioctl(inet_sock_fd,SIOCSIFFLAGS,&ifr);
}

//http://lxr.free-electrons.com/source/Documentation/networking/ifenslave.c#L990
NAN_METHOD(IfUp) {
  NanScope();

  String::Utf8Value iface(args[0]);

  int res = ifUp(*iface);

  NanReturnValue(NanNew<Integer>(res));
}

int setDefGateway(const char * defGateway) {
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
  if ((err = ioctl(inet_sock_fd, SIOCADDRT, &rm)) < 0) {
    printf("SIOCADDRT failed , ret->%d\n",err);
    return -1;
  }
  return 0;
}

// http://pic.dhe.ibm.com/infocenter/aix/v7r1/index.jsp?topic=%2Fcom.ibm.aix.commtechref%2Fdoc%2Fcommtrf2%2Fioctl_socket_control_operations.htm
NAN_METHOD(SetDefaultGateway) {
  NanScope();

  String::Utf8Value ip(args[0]);

  int res = setDefGateway(*ip);

  NanReturnValue(NanNew<Integer>(res));
}

void init(Handle<Object> exports) {
  exports->Set(NanNew<String>("flags"),
    NanNew<FunctionTemplate>(GetFlags)->GetFunction());
  exports->Set(NanNew<String>("ioctl"),
    NanNew<FunctionTemplate>(Ioctl)->GetFunction());
  exports->Set(NanNew<String>("getHardwareAddress"),
    NanNew<FunctionTemplate>(GetHardwareAddress)->GetFunction());
  exports->Set(NanNew<String>("getNetmasks"),
    NanNew<FunctionTemplate>(GetNetmasks)->GetFunction());
  exports->Set(NanNew<String>("getAddresses"),
    NanNew<FunctionTemplate>(GetAddresses)->GetFunction());
  exports->Set(NanNew<String>("setDefaultGateway"),
    NanNew<FunctionTemplate>(SetDefaultGateway)->GetFunction());
  exports->Set(NanNew<String>("setAddress"),
    NanNew<FunctionTemplate>(SetAddress)->GetFunction());
  exports->Set(NanNew<String>("ifup"),
    NanNew<FunctionTemplate>(IfUp)->GetFunction());
  exports->Set(NanNew<String>("loopbackUp"),
    NanNew<FunctionTemplate>(LoopbackUp)->GetFunction());
}

NODE_MODULE(binding, init)
