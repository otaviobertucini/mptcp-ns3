#ifndef ATTACKER_SOCKET_FACTORY_H
#define ATTACKER_SOCKET_FACTORY_H

#include "ns3/socket-factory.h"

namespace ns3
{

class Socket;

class AttackerSocketFactory : public SocketFactory
{
public:
  static TypeId GetTypeId();
};

}

#endif 
