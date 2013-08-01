/*
 * Copyright (C) 2012 Jolla Ltd.
 * Contact: Aaron McCarthy <aaron.mccarthy@jollamobile.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/

#include <assert.h>
#include <stdio.h>

#include <hardware/hardware.h>
#include <hardware/nfc.h>
#include <libnfc-nxp/phLibNfc.h>
#include <libnfc-nxp/phDal4Nfc_messageQueueLib.h>

void initializeCallback(void *pContext, NFCSTATUS status)
{
    NFCSTATUS *callbackStatus = (NFCSTATUS *)pContext;
    *callbackStatus = status;
}

void processMessage(unsigned int clientId)
{
    phDal4Nfc_Message_Wrapper_t message;
    int ret = phDal4Nfc_msgrcv(clientId, &message, sizeof(phLibNfc_Message_t), 0, 0);
    if (ret == -1) {
        printf("Failed to receive message from NFC stack.\n");
        assert(1);
        return;
    }

    switch (message.msg.eMsgType) {
    case PH_LIBNFC_DEFERREDCALL_MSG: {
        phLibNfc_DeferredCall_t *msg = (phLibNfc_DeferredCall_t *)message.msg.pMsgData;
        if (msg->pCallback)
            msg->pCallback(msg->pParameter);
        break;
    }
    default:
        printf("Unknown message type %d.\n", message.msg.eMsgType);
    }
}

int main(int argc, char **argv)
{
    printf("Starting test_nfc.\n");

    const hw_module_t *hwModule = 0;
    nfc_pn544_device_t *nfcDevice = 0;

    printf("Finding NFC hardware module.\n");
    hw_get_module(NFC_HARDWARE_MODULE_ID, &hwModule);
    assert(hwModule != NULL);

    printf("Opening NFC device.\n");
    assert(nfc_pn544_open(hwModule, &nfcDevice) == 0);
    assert(nfcDevice != 0);

    assert(nfcDevice->num_eeprom_settings != 0);
    assert(nfcDevice->eeprom_settings);

    printf("Configuring NFC driver.\n");
    phLibNfc_sConfig_t driverConfig;
    driverConfig.nClientId = phDal4Nfc_msgget(0, 0600);
    assert(driverConfig.nClientId);

    void *hwRef;
    NFCSTATUS status = phLibNfc_Mgt_ConfigureDriver(&driverConfig, &hwRef);
    assert(hwRef);
    assert(status == NFCSTATUS_SUCCESS);

    printf("Initializing NFC stack.\n");
    NFCSTATUS callbackStatus = 0xFFFF;
    status = phLibNfc_Mgt_Initialize(hwRef, initializeCallback, &callbackStatus);
    assert(status == NFCSTATUS_PENDING);

    while (callbackStatus == 0xFFFF)
        processMessage(driverConfig.nClientId);

    assert(callbackStatus == NFCSTATUS_SUCCESS);

    printf("Getting NFC stack capabilities.\n");
    phLibNfc_StackCapabilities_t capabilities;
    status = phLibNfc_Mgt_GetstackCapabilities(&capabilities, &callbackStatus);
    assert(status == NFCSTATUS_SUCCESS);

    printf("NFC capabilities:\n"
           "\tHAL version: %u\n"
           "\tFW version: %u\n"
           "\tHW version: %u\n"
           "\tModel: %u\n"
           "\tHCI version: %u\n"
           "\tVendor: %s\n"
           "\tFull version: %u %u\n"
           "\tFW Update: %u\n",
           capabilities.psDevCapabilities.hal_version, capabilities.psDevCapabilities.fw_version,
           capabilities.psDevCapabilities.hw_version, capabilities.psDevCapabilities.model_id,
           capabilities.psDevCapabilities.hci_version,
           capabilities.psDevCapabilities.vendor_name,
           capabilities.psDevCapabilities.full_version[NXP_FULL_VERSION_LEN-1],
           capabilities.psDevCapabilities.full_version[NXP_FULL_VERSION_LEN-2],
           capabilities.psDevCapabilities.firmware_update_info);

    printf("Deinitializing NFC stack.\n");
    callbackStatus = 0xFFFF;
    status = phLibNfc_Mgt_DeInitialize(hwRef, initializeCallback, &callbackStatus);
    assert(status == NFCSTATUS_PENDING);

    while (callbackStatus == 0xFFFF)
        processMessage(driverConfig.nClientId);

    assert(callbackStatus == NFCSTATUS_SUCCESS);

    printf("Unconfiguring NFC driver.\n");
    status = phLibNfc_Mgt_UnConfigureDriver(hwRef);
    assert(status == NFCSTATUS_SUCCESS);

    int result = phDal4Nfc_msgctl(driverConfig.nClientId, 0, 0);
    assert(result == 0);

    printf("Closing NFC device.\n");
    nfc_pn544_close(nfcDevice);

	return 0;
}
