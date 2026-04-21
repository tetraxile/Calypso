#include <netinet/in.h>
#include "hk/Result.h"

namespace nn::nifm {
hk::Result GetCurrentIpConfigInfo(in_addr* currentAddr, in_addr* subnetMask, in_addr* gateway, in_addr* primaryDNSServer, in_addr* secondaryDNSServer);
}
