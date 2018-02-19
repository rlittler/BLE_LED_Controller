/* mbed Microcontroller Library
 * Copyright (c) 2006-2013 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mbed.h"
#include "BLEDevice.h"

#include "UARTService.h"

#define NEED_CONSOLE_OUTPUT 0 /* Set this if you need debug messages on the console;
                               * it will have an impact on code-size and power consumption. */

#if NEED_CONSOLE_OUTPUT
#define DEBUG(...) { printf(__VA_ARGS__); }
#else
#define DEBUG(...) /* nothing */
#endif /* #if NEED_CONSOLE_OUTPUT */

#define LED2CMD "led2"
#define CMD_LENGTH 4


//char rxPayload[CMD_SIZE];

BLEDevice  ble;                               // Create Bluetooth object
DigitalOut led1(LED1);                        // Set the pin attached to LED1 as an output
PwmOut led2(LED2);                        // Set the pin attached to LED2 as an output

UARTService *uartServicePtr;

/* BLE disconnected callback */
void disconnectionCallback(Gap::Handle_t handle, Gap::DisconnectionReason_t reason)
{
    DEBUG("Disconnected!\n\r");
    DEBUG("Restarting the advertising process\n\r");
    ble.startAdvertising();
}
/* BLE UART data received callback */
void onDataWritten(const GattCharacteristicWriteCBParams *params)
{
    if ((uartServicePtr != NULL) && (params->charHandle == uartServicePtr->getTXCharacteristicHandle())) {     //If characters received over BLE
        uint16_t bytesRead = params->len;
        DEBUG("received %u bytes\n\r", bytesRead);
        DEBUG("Received string: '");
        DEBUG((const char *)params->data);             //Note the size of data expands to the largest string received. Need to use bytesRead to resize.
        DEBUG("'\n\r");
        if (!strncmp(LED2CMD,(const char *)params->data,CMD_LENGTH-1)){
            float value;
            char cmd[CMD_LENGTH];
            sscanf((const char *)params->data, "%s %f", cmd, &value );
            led2 = value;
            DEBUG("Cmd: %s LED Level = %f\n\r", cmd, value);
            }
        ble.updateCharacteristicValue(uartServicePtr->getRXCharacteristicHandle(), params->data,bytesRead);   // Echo received characters back over BLE
    }
}

/* Periodic Ticker callback */
void periodicCallback(void)
{
    led1 = !led1;                             // Toggle LED 1
}

int main(void)
{
    led1 = 1;
    led2 = 0.5;
    Ticker ticker;                            // Create period timer
    ticker.attach(periodicCallback, 1);       // Attach ticker callback function with a period of 1 second

    DEBUG("Initialising the nRF51822\n\r");
    ble.init();
    ble.onDisconnection(disconnectionCallback);                                            // Define callback function for BLE disconnection event
    ble.onDataWritten(onDataWritten);                                                      // Define callback function for BLE Data received event

    /* setup advertising */
    ble.accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED);             // Indicate that Legacy Bluetooth in not supported
    ble.setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
    ble.accumulateAdvertisingPayload(GapAdvertisingData::SHORTENED_LOCAL_NAME,
                                     (const uint8_t *)"BLE UART", sizeof("BLE UART") - 1);
    ble.accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_128BIT_SERVICE_IDS,
                                     (const uint8_t *)UARTServiceUUID_reversed, sizeof(UARTServiceUUID_reversed));

    ble.setAdvertisingInterval(Gap::MSEC_TO_ADVERTISEMENT_DURATION_UNITS(1000));          // Set advertising interval to 1 second
    ble.startAdvertising();                                                               // Start advertising

    UARTService uartService(ble);                                                         // Create BLE UART service
    uartServicePtr = &uartService;                                                        // Initalise pointer to point to UART Service

    while (true) {
        ble.waitForEvent();                                                               // Wait for BLE events
    }
}
