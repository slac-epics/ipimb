/* drvIPIMBRegister.cpp */

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "iocsh.h"
#include "drvIpimb.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

int IPIMB_DRV_DEBUG=0;
int IPIMB_DEV_DEBUG=0;

static const iocshArg IPIMB_DRV_DEBUGArg0 = {"value", iocshArgInt};
static const iocshArg *const IPIMB_DRV_DEBUGArgs[1] = {&IPIMB_DRV_DEBUGArg0};
static const iocshFuncDef IPIMB_DRV_DEBUGDef = {"IPIMB_DRV_DEBUG", 1, IPIMB_DRV_DEBUGArgs};
static void IPIMB_DRV_DEBUGCall(const iocshArgBuf * args) {
        IPIMB_DRV_DEBUG = args[0].ival;
}

static const iocshArg IPIMB_DEV_DEBUGArg0 = {"value", iocshArgInt};
static const iocshArg *const IPIMB_DEV_DEBUGArgs[1] = {&IPIMB_DEV_DEBUGArg0};
static const iocshFuncDef IPIMB_DEV_DEBUGDef = {"IPIMB_DEV_DEBUG", 1, IPIMB_DEV_DEBUGArgs};
static void IPIMB_DEV_DEBUGCall(const iocshArgBuf * args) {
        IPIMB_DEV_DEBUG = args[0].ival;
}

static const iocshArg ipimbAddArg0 = {"name", iocshArgString};
static const iocshArg ipimbAddArg1 = {"ttyName", iocshArgString};
static const iocshArg ipimbAddArg2 = {"mdestIP", iocshArgString};
static const iocshArg ipimbAddArg3 = {"physID", iocshArgInt};
static const iocshArg ipimbAddArg4 = {"datatype", iocshArgInt};
static const iocshArg *const ipimbAddArgs[5] = {&ipimbAddArg0, &ipimbAddArg1, &ipimbAddArg2, &ipimbAddArg3,
                                                &ipimbAddArg4};
static const iocshFuncDef ipimbAddDef = {"ipimbAdd", 5, ipimbAddArgs};
static void ipimbAddCall(const iocshArgBuf * args) {
    ipimbAdd( (char *)(args[0].sval), (char *)(args[1].sval), (char *)(args[2].sval),
              (unsigned int) args[3].ival, (unsigned int) args[4].ival );
}

void drvIPIMB_Register() {
        static int firstTime = 1;
        if  (!firstTime)
            return;
        firstTime = 0;
        iocshRegister(&IPIMB_DRV_DEBUGDef, IPIMB_DRV_DEBUGCall);
        iocshRegister(&IPIMB_DEV_DEBUGDef, IPIMB_DEV_DEBUGCall);
        iocshRegister(&ipimbAddDef, ipimbAddCall);
}
#ifdef __cplusplus
}
#endif  /* __cplusplus */
/*
 * Register commands on application startup
 * In the funture we might change this to xxx = drvIPIMB_Register(); to guarantee link
 */
class drvIPIMB_iocshReg {
    public:
    drvIPIMB_iocshReg() {
        drvIPIMB_Register();
    }
};
static drvIPIMB_iocshReg drvIPIMB_iocshRegObj;
