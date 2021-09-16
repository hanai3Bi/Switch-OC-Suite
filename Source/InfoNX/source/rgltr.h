#pragma once
//#include "../types.h"
//#include "../sf/service.h"
//#include "../services/pcv.h"
#include <switch.h>
#include "pcv_types.h"

typedef struct {
    Service s;
} RgltrSession;

Result rgltrInitialize(void);

void rgltrExit(void);

Service* rgltrGetServiceSession(void);

Result rgltrOpenSession(RgltrSession* session_out, PowerDomainId module_id);
void rgltrCloseSession(RgltrSession* session);
Result rgltrGetVoltage(RgltrSession* session, u32 *out_volt);
Result rgltrGetPowerModuleNumLimit(u32 *out);
Result rgltrGetVoltageEnabled(RgltrSession* session, u32 *out);