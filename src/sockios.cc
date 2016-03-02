#include <iostream>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/route.h>
#include <linux/sockios.h>
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
  Nan::HandleScope scope;
  Local<Object> flags = Nan::New<Object>();

  flags->Set( Nan::New<String>("SIOCGIFNAME").ToLocalChecked(), Nan::New<Integer>(SIOCGIFNAME) );
  flags->Set( Nan::New<String>("SIOCGIFINDEX").ToLocalChecked(), Nan::New<Integer>(SIOCGIFINDEX) );
  flags->Set( Nan::New<String>("SIOCGIFFLAGS").ToLocalChecked(), Nan::New<Integer>(SIOCGIFFLAGS) );
  flags->Set( Nan::New<String>("SIOCSIFFLAGS").ToLocalChecked(), Nan::New<Integer>(SIOCSIFFLAGS) );
  flags->Set( Nan::New<String>("SIOCGIFPFLAGS").ToLocalChecked(), Nan::New<Integer>(SIOCGIFPFLAGS) );
  flags->Set( Nan::New<String>("SIOCSIFPFLAGS").ToLocalChecked(), Nan::New<Integer>(SIOCSIFPFLAGS) );
  flags->Set( Nan::New<String>("SIOCGIFADDR").ToLocalChecked(), Nan::New<Integer>(SIOCGIFADDR) );
  flags->Set( Nan::New<String>("SIOCSIFADDR").ToLocalChecked(), Nan::New<Integer>(SIOCSIFADDR) );
  flags->Set( Nan::New<String>("SIOCGIFDSTADDR").ToLocalChecked(), Nan::New<Integer>(SIOCGIFDSTADDR) );
  flags->Set( Nan::New<String>("SIOCSIFDSTADDR").ToLocalChecked(), Nan::New<Integer>(SIOCSIFDSTADDR) );
  flags->Set( Nan::New<String>("SIOCGIFBRDADDR").ToLocalChecked(), Nan::New<Integer>(SIOCGIFBRDADDR) );
  flags->Set( Nan::New<String>("SIOCSIFBRDADDR").ToLocalChecked(), Nan::New<Integer>(SIOCSIFBRDADDR) );
  flags->Set( Nan::New<String>("SIOCGIFNETMASK").ToLocalChecked(), Nan::New<Integer>(SIOCGIFNETMASK) );
  flags->Set( Nan::New<String>("SIOCSIFNETMASK").ToLocalChecked(), Nan::New<Integer>(SIOCSIFNETMASK) );
  flags->Set( Nan::New<String>("SIOCGIFMETRIC").ToLocalChecked(), Nan::New<Integer>(SIOCGIFMETRIC) );
  flags->Set( Nan::New<String>("SIOCSIFMETRIC").ToLocalChecked(), Nan::New<Integer>(SIOCSIFMETRIC) );
  flags->Set( Nan::New<String>("SIOCGIFMTU").ToLocalChecked(), Nan::New<Integer>(SIOCGIFMTU) );
  flags->Set( Nan::New<String>("SIOCSIFMTU").ToLocalChecked(), Nan::New<Integer>(SIOCSIFMTU) );
  flags->Set( Nan::New<String>("SIOCGIFHWADDR").ToLocalChecked(), Nan::New<Integer>(SIOCGIFHWADDR) );
  flags->Set( Nan::New<String>("SIOCSIFHWADDR").ToLocalChecked(), Nan::New<Integer>(SIOCSIFHWADDR) );
  flags->Set( Nan::New<String>("SIOCSIFHWBROADCAST").ToLocalChecked(), Nan::New<Integer>(SIOCSIFHWBROADCAST) );
  flags->Set( Nan::New<String>("SIOCGIFMAP").ToLocalChecked(), Nan::New<Integer>(SIOCGIFMAP) );
  flags->Set( Nan::New<String>("SIOCSIFMAP").ToLocalChecked(), Nan::New<Integer>(SIOCSIFMAP) );
  flags->Set( Nan::New<String>("SIOCADDMULTI").ToLocalChecked(), Nan::New<Integer>(SIOCADDMULTI) );
  flags->Set( Nan::New<String>("SIOCGIFTXQLEN").ToLocalChecked(), Nan::New<Integer>(SIOCGIFTXQLEN) );
  flags->Set( Nan::New<String>("SIOCSIFTXQLEN").ToLocalChecked(), Nan::New<Integer>(SIOCSIFTXQLEN) );
  flags->Set( Nan::New<String>("SIOCSIFNAME").ToLocalChecked(), Nan::New<Integer>(SIOCSIFNAME) );
  flags->Set( Nan::New<String>("SIOCGIFCONF").ToLocalChecked(), Nan::New<Integer>(SIOCGIFCONF) );

  info.GetReturnValue().Set(flags);
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
  Nan::HandleScope scope;
  ifreq req;
  char out [18];

  String::Utf8Value name(info[0]);

  strcpy( req.ifr_name, *name );

  ioctl(inet_sock_fd, SIOCGIFHWADDR, &req);

  char* hwaddr = req.ifr_hwaddr.sa_data;

  sprintf(out,ether_fmt,hwaddr[0],hwaddr[1],hwaddr[2],hwaddr[3],hwaddr[4],hwaddr[5]);

  info.GetReturnValue().Set(Nan::New<String>(out).ToLocalChecked());
}

NAN_METHOD(Ioctl) {
  Nan::HandleScope scope;

  int sfd = Local<Number>::Cast(info[0])->Value();
  int req = Local<Number>::Cast(info[1])->Value();

  ifreq ifreq;

  int res = ioctl( sfd, req, &ifreq );

  info.GetReturnValue().Set(Nan::New<Integer>(res));
}

// format and return IP address as dotted decimal
// from a sockaddr object
char* FormatIP(sockaddr *sock_addr){
  return inet_ntoa(((struct sockaddr_in*) sock_addr)->sin_addr);
}

NAN_METHOD(GetNetmasks) {
  Nan::HandleScope scope;
  ifreq req;

  String::Utf8Value name(info[0]);
  strcpy( req.ifr_name, *name );
  ioctl(inet_sock_fd, SIOCGIFNETMASK, &req);
  char* addr = FormatIP(&req.ifr_netmask);

  info.GetReturnValue().Set(Nan::New<String>(addr).ToLocalChecked());
}

NAN_METHOD(GetAddresses) {
  Nan::HandleScope scope;

  int buff_count = 10;
  int IFREQ_SIZE = sizeof( ifreq );

  ifconf ifaces;
  ifaces.ifc_req = new ifreq[buff_count];
  ifaces.ifc_len = buff_count * IFREQ_SIZE;

  ioctl(inet_sock_fd,SIOCGIFCONF,&ifaces);
  int len = ifaces.ifc_len / IFREQ_SIZE;

  Local<Object> obj = Nan::New<Object>();

  for(int i=0; i<len; i++){
    ifreq req = ifaces.ifc_req[i];

    struct sockaddr *saddr = &req.ifr_addr;

    char* addr = FormatIP(saddr);

    Local<Object> iface = Nan::New<Object>();

    char* name = req.ifr_name;

    iface->Set(Nan::New<String>("address").ToLocalChecked(), Nan::New<String>(addr).ToLocalChecked());

    obj->Set(Nan::New<String>(name).ToLocalChecked(), iface);
  }

  // free memory
  delete ifaces.ifc_req;
  info.GetReturnValue().Set(obj);
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
  Nan::HandleScope scope;

  String::Utf8Value iface(info[0]);
  String::Utf8Value ip(info[1]);

  int res = setAddress(*iface, *ip);

  info.GetReturnValue().Set(Nan::New<Integer>(res));
}


NAN_METHOD(LoopbackUp) {
  Nan::HandleScope scope;

  struct ifreq ifr;
  int res = 0;
  ifr.ifr_flags = IFF_UP|IFF_LOOPBACK|IFF_RUNNING;
  strncpy(ifr.ifr_name,loopbackName,IFNAMSIZ);

  res = ioctl(inet_sock_fd,SIOCSIFFLAGS,&ifr);

  info.GetReturnValue().Set(Nan::New<Integer>(res));
}

int ifUp(char* iface) {
  struct ifreq ifr;

  ifr.ifr_flags = IFF_UP|IFF_LOOPBACK|IFF_RUNNING;
  strncpy(ifr.ifr_name,iface,IFNAMSIZ);

  return ioctl(inet_sock_fd,SIOCSIFFLAGS,&ifr);
}

//http://lxr.free-electrons.com/source/Documentation/networking/ifenslave.c#L990
NAN_METHOD(IfUp) {
  Nan::HandleScope scope;

  String::Utf8Value iface(info[0]);

  int res = ifUp(*iface);

  info.GetReturnValue().Set(Nan::New<Integer>(res));
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
  Nan::HandleScope scope;

  String::Utf8Value ip(info[0]);

  int res = setDefGateway(*ip);

  info.GetReturnValue().Set(Nan::New<Integer>(res));
}

void init(Local<Object> exports) {
  exports->Set(Nan::New<String>("flags").ToLocalChecked(),
    Nan::New<FunctionTemplate>(GetFlags)->GetFunction());
  exports->Set(Nan::New<String>("ioctl").ToLocalChecked(),
    Nan::New<FunctionTemplate>(Ioctl)->GetFunction());
  exports->Set(Nan::New<String>("getHardwareAddress").ToLocalChecked(),
    Nan::New<FunctionTemplate>(GetHardwareAddress)->GetFunction());
  exports->Set(Nan::New<String>("getNetmasks").ToLocalChecked(),
    Nan::New<FunctionTemplate>(GetNetmasks)->GetFunction());
  exports->Set(Nan::New<String>("getAddresses").ToLocalChecked(),
    Nan::New<FunctionTemplate>(GetAddresses)->GetFunction());
  exports->Set(Nan::New<String>("setDefaultGateway").ToLocalChecked(),
    Nan::New<FunctionTemplate>(SetDefaultGateway)->GetFunction());
  exports->Set(Nan::New<String>("setAddress").ToLocalChecked(),
    Nan::New<FunctionTemplate>(SetAddress)->GetFunction());
  exports->Set(Nan::New<String>("ifup").ToLocalChecked(),
    Nan::New<FunctionTemplate>(IfUp)->GetFunction());
  exports->Set(Nan::New<String>("loopbackUp").ToLocalChecked(),
    Nan::New<FunctionTemplate>(LoopbackUp)->GetFunction());
}

NODE_MODULE(binding, init)
