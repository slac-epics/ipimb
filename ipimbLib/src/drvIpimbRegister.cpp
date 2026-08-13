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
int IPIMB_BRD_DEBUG=0;
int IPIMB_BRD_ID=-1;

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

static const iocshArg IPIMB_BRD_DEBUGArg0 = {"value", iocshArgInt};
static const iocshArg *const IPIMB_BRD_DEBUGArgs[1] = {&IPIMB_BRD_DEBUGArg0};
static const iocshFuncDef IPIMB_BRD_DEBUGDef = {"IPIMB_BRD_DEBUG", 1, IPIMB_BRD_DEBUGArgs};
static void IPIMB_BRD_DEBUGCall(const iocshArgBuf * args) {
        IPIMB_BRD_DEBUG = args[0].ival;
}

static const iocshArg IPIMB_BRD_IDArg0 = {"value", iocshArgInt};
static const iocshArg *const IPIMB_BRD_IDArgs[1] = {&IPIMB_BRD_IDArg0};
static const iocshFuncDef IPIMB_BRD_IDDef = {"IPIMB_BRD_ID", 1, IPIMB_BRD_IDArgs};
static void IPIMB_BRD_IDCall(const iocshArgBuf * args) {
        IPIMB_BRD_ID = args[0].ival;
}

static const iocshArg ipimbAddArg0 = {"name", iocshArgString};
static const iocshArg ipimbAddArg1 = {"ttyName", iocshArgString};
static const iocshArg ipimbAddArg2 = {"mdestIP", iocshArgString};
static const iocshArg ipimbAddArg3 = {"physID", iocshArgInt};
static const iocshArg ipimbAddArg4 = {"datatype", iocshArgInt};
static const iocshArg ipimbAddArg5 = {"gen", iocshArgString};
static const iocshArg ipimbAddArg6 = {"trigger", iocshArgString};
static const iocshArg ipimbAddArg7 = {"polarity", iocshArgInt};
static const iocshArg ipimbAddArg8 = {"delay", iocshArgString};
static const iocshArg ipimbAddArg9 = {"sync", iocshArgString};
static const iocshArg *const ipimbAddArgs[9] = {&ipimbAddArg0, &ipimbAddArg1, &ipimbAddArg2, &ipimbAddArg3,
                                                &ipimbAddArg4, &ipimbAddArg6, &ipimbAddArg7, &ipimbAddArg8,
                                                &ipimbAddArg9};
static const iocshFuncDef ipimbAddDef = {"ipimbAdd", 9, ipimbAddArgs};
static void ipimbAddCall(const iocshArgBuf * args) {
    int retcode = 0;
    // null check all the string arguments
    for (int idx=0; idx<ipimbAddDef.nargs; idx++) {
      if ((ipimbAddArgs[idx]->type == iocshArgString) && !args[idx].sval) {
        printf("%s string argument %s (%d) must be non-null!\n",
               ipimbAddDef.name, ipimbAddArgs[idx]->name, idx);
        retcode = 1;
      }
    }
    if (retcode == 0) {
      retcode = ipimbAdd( args[0].sval, args[1].sval, args[2].sval,
                          (unsigned int) args[3].ival, (unsigned int) args[4].ival,
                          NULL, args[6].sval, args[7].ival,
                          args[8].sval, args[9].sval);
    }
    iocshSetError(retcode);
}

static const iocshArg *const ipimbTprAddArgs[10] = {&ipimbAddArg0, &ipimbAddArg1, &ipimbAddArg2, &ipimbAddArg3,
                                                    &ipimbAddArg4, &ipimbAddArg5, &ipimbAddArg6, &ipimbAddArg7,
                                                    &ipimbAddArg8, &ipimbAddArg9};
static const iocshFuncDef ipimbTprAddDef = {"ipimbTprAdd", 10, ipimbTprAddArgs};
static void ipimbTprAddCall(const iocshArgBuf * args) {
    int retcode = 0;
    // null check all the string arguments
    for (int idx=0; idx<ipimbTprAddDef.nargs; idx++) {
      if ((ipimbTprAddArgs[idx]->type == iocshArgString) && !args[idx].sval) {
        printf("%s string argument %s (%d) must be non-null!\n",
               ipimbTprAddDef.name, ipimbTprAddArgs[idx]->name, idx);
        retcode = 1;
      }
    }
    if (retcode == 0) {
      retcode = ipimbAdd( args[0].sval, args[1].sval, args[2].sval,
                          (unsigned int) args[3].ival, (unsigned int) args[4].ival,
                          args[5].sval, args[6].sval, args[7].ival,
                          args[8].sval, args[9].sval);
    }
    iocshSetError(retcode);
}

static const iocshFuncDef ipimbStartDef = {"ipimbStart", 0, NULL};
static void ipimbStartCall(const iocshArgBuf * args) {
    ipimbStart();
}

void drvIPIMB_Register() {
        static int firstTime = 1;
        if  (!firstTime)
            return;
        firstTime = 0;
        iocshRegister(&IPIMB_DRV_DEBUGDef, IPIMB_DRV_DEBUGCall);
        iocshRegister(&IPIMB_DEV_DEBUGDef, IPIMB_DEV_DEBUGCall);
        iocshRegister(&IPIMB_BRD_DEBUGDef, IPIMB_BRD_DEBUGCall);
        iocshRegister(&IPIMB_BRD_IDDef,    IPIMB_BRD_IDCall);
        iocshRegister(&ipimbAddDef, ipimbAddCall);
        iocshRegister(&ipimbTprAddDef, ipimbTprAddCall);
        iocshRegister(&ipimbStartDef, ipimbStartCall);
}
#ifdef __cplusplus
}
#endif  /* __cplusplus */
/*
 * Register commands on application startup
 * In the future we might change this to xxx = drvIPIMB_Register(); to guarantee link
 */
class drvIPIMB_iocshReg {
    public:
    drvIPIMB_iocshReg() {
        drvIPIMB_Register();
    }
};
static drvIPIMB_iocshReg drvIPIMB_iocshRegObj;
