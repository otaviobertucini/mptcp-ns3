#include "attacker-socket-factory.h"

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(AttackerSocketFactory);

TypeId
AttackerSocketFactory::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::AttackerSocketFactory")
      .SetParent<SocketFactory>();
  return tid;
}

} // namespace ns3
