/*
 * Copyright 2010-2015 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

/**
 * @file shadow_sample.c
 * @brief A simple connected window example demonstrating the use of Thing
 * Shadow
 */

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/time.h>
#include <wiringPi.h>

#include "aws_iot_config.h"
#include "aws_iot_log.h"
#include "aws_iot_mqtt_client_interface.h"
#include "aws_iot_shadow_interface.h"
#include "aws_iot_version.h"
#include "aws_iot_json_utils.h"


/*!
 * The goal of this sample application is to demonstrate the capabilities of
   shadow.
 * This device(say Connected Window) will open the window of a room based on
   temperature
 * It can report to the Shadow the following parameters:
 *  1. temperature of the room (double)
 *  2. status of the window (open or close)
 * It can act on commands from the cloud. In this case it will open or close the
   window based on the json object "windowOpen" data[open/close]
 *
 * The two variables from a device's perspective are double temperature and bool
   windowOpen
 * The device needs to act on only on windowOpen variable, so we will create a
   primitiveJson_t object with callback The Json Document in the cloud will be
   {
   "reported": {
   "buttonPressed":true
   },
   "desired": {
   "buttonPressed":false
   }
   }
 */
/******************************************************************************/
// Button Definitions
#define PIN_BUTTON_1 1
#define PIN_BUTTON_2 2
#define PIN_BUTTON_3 3
#define TOTAL_BUTTONS 3
// How much time a change must be since the last in order to count as a change
#define IGNORE_CHANGE_BELOW_USEC 200000
// Current state of the pin
static volatile bool buttonTriggers[TOTAL_BUTTONS]={0};
static uint8_t buttonEnabledArray[TOTAL_BUTTONS];
static uint8_t buttonPinArray[TOTAL_BUTTONS]={PIN_BUTTON_1,PIN_BUTTON_2,PIN_BUTTON_3};
static struct timeval lastChange[TOTAL_BUTTONS];

// initialize the mqtt client
static AWS_IoT_Client mqttClient;
/******************************************************************************/
/*
 * @note The delta message is always sent on the "state" key in the json
 * @note Any time messages are bigger than AWS_IOT_MQTT_RX_BUF_LEN the
 * underlying MQTT library will ignore it. The maximum size of the message that
 * can be received is limited to the AWS_IOT_MQTT_RX_BUF_LEN
 */
char stringToEchoDelta[SHADOW_MAX_SIZE_OF_RX_BUFFER];
bool messageArrivedOnDelta = false;

#define ROOMTEMPERATURE_UPPERLIMIT 32.0f
#define ROOMTEMPERATURE_LOWERLIMIT 25.0f
#define STARTING_ROOMTEMPERATURE ROOMTEMPERATURE_LOWERLIMIT

#define MAX_LENGTH_OF_UPDATE_JSON_BUFFER 200

static char certDirectory[PATH_MAX + 1] = "../../../certs";
#define HOST_ADDRESS_SIZE 255
static char HostAddress[HOST_ADDRESS_SIZE] = AWS_IOT_MQTT_HOST;
static uint32_t port = AWS_IOT_MQTT_PORT;
static uint8_t numPubs = 5;


/******************************************************************************/
//JSON parsing
static jsmn_parser jsonParser;
static jsmntok_t jsonTokenStruct[MAX_JSON_TOKEN_EXPECTED];
static int32_t tokenCount;



// Helper functions
void parseInputArgsForConnectParams(int argc, char **argv);

// Shadow Callback for receiving the delta
void DeltaCallback(const char *pJsonValueBuffer, uint32_t valueLength,
                   jsonStruct_t *pJsonStruct_t);

void UpdateStatusCallback(const char *pThingName, ShadowActions_t action,
                          Shadow_Ack_Status_t status,
                          const char *pReceivedJsonDocument,
                          void *pContextData);

// Handler for button interrupt
void buttonCallbackHandler(int i);
// Handler for button interrupt
void buttonCallback1(void){
        buttonCallbackHandler(0);
}
void buttonCallback2(void){
        buttonCallbackHandler(1);
}
void buttonCallback3(void){
        buttonCallbackHandler(2);
}
void *buttonCallback[TOTAL_BUTTONS] = {&buttonCallback1, &buttonCallback2,&buttonCallback3};
bool buttonInterruptCheck(void);
int buttonPressedMask(void);
void cleanButtonInterrupts(void);
int buttonEnabledMask(void);

int main(int argc, char **argv) {

        IoT_Error_t rc = FAILURE;
        int32_t i = 0;

        char JsonDocumentBuffer[MAX_LENGTH_OF_UPDATE_JSON_BUFFER];
        size_t sizeOfJsonDocumentBuffer =
                sizeof(JsonDocumentBuffer) / sizeof(JsonDocumentBuffer[0]);
        char *pJsonStringToUpdate;

        uint8_t buttonPressed = 0;
        jsonStruct_t buttonHandler;
        buttonHandler.pKey = "buttonPressed";
        buttonHandler.pData = &buttonPressed;
        buttonHandler.dataLength = sizeof(uint8_t);
        buttonHandler.type = SHADOW_JSON_INT8;

        char rootCA[PATH_MAX + 1];
        char clientCRT[PATH_MAX + 1];
        char clientKey[PATH_MAX + 1];
        char CurrentWD[PATH_MAX + 1];

        IOT_INFO("\nAWS IoT SDK Version %d.%d.%d-%s\n", VERSION_MAJOR, VERSION_MINOR,
                 VERSION_PATCH, VERSION_TAG);

        getcwd(CurrentWD, sizeof(CurrentWD));
        snprintf(rootCA, PATH_MAX + 1, "%s/%s/%s", CurrentWD, certDirectory,
                 AWS_IOT_ROOT_CA_FILENAME);
        snprintf(clientCRT, PATH_MAX + 1, "%s/%s/%s", CurrentWD, certDirectory,
                 AWS_IOT_CERTIFICATE_FILENAME);
        snprintf(clientKey, PATH_MAX + 1, "%s/%s/%s", CurrentWD, certDirectory,
                 AWS_IOT_PRIVATE_KEY_FILENAME);

        IOT_DEBUG("rootCA %s", rootCA);
        IOT_DEBUG("clientCRT %s", clientCRT);
        IOT_DEBUG("clientKey %s", clientKey);

        parseInputArgsForConnectParams(argc, argv);

        ShadowInitParameters_t sp = ShadowInitParametersDefault;
        sp.pHost = AWS_IOT_MQTT_HOST;
        sp.port = AWS_IOT_MQTT_PORT;
        sp.pClientCRT = clientCRT;
        sp.pClientKey = clientKey;
        sp.pRootCA = rootCA;
        sp.enableAutoReconnect = false;
        sp.disconnectHandler = NULL;

        IOT_INFO("Shadow Init");
        rc = aws_iot_shadow_init(&mqttClient, &sp);
        if (SUCCESS != rc) {
                IOT_ERROR("Shadow Connection Error");
                return rc;
        }

        ShadowConnectParameters_t scp = ShadowConnectParametersDefault;
        scp.pMyThingName = AWS_IOT_MY_THING_NAME;
        scp.pMqttClientId = AWS_IOT_MQTT_CLIENT_ID;
        scp.mqttClientIdLen = (uint16_t)strlen(AWS_IOT_MQTT_CLIENT_ID);

        IOT_INFO("Shadow Connect");
        rc = aws_iot_shadow_connect(&mqttClient, &scp);
        if (SUCCESS != rc) {
                IOT_ERROR("Shadow Connection Error");
                return rc;
        }

        /*
         * Enable Auto Reconnect functionality. Minimum and Maximum time of
         * Exponential backoff are set in aws_iot_config.h
         *  #AWS_IOT_MQTT_MIN_RECONNECT_WAIT_INTERVAL
         *  #AWS_IOT_MQTT_MAX_RECONNECT_WAIT_INTERVAL
         */
        rc = aws_iot_shadow_set_autoreconnect_status(&mqttClient, true);
        if (SUCCESS != rc) {
                IOT_ERROR("Unable to set Auto Reconnect to true - %d", rc);
                return rc;
        }
        jsonStruct_t deltaObject;
        deltaObject.pData = stringToEchoDelta;
        deltaObject.dataLength = SHADOW_MAX_SIZE_OF_RX_BUFFER;
        deltaObject.pKey = "state";
        deltaObject.type = SHADOW_JSON_OBJECT;
        deltaObject.cb = DeltaCallback;

        rc = aws_iot_shadow_register_delta(&mqttClient, &deltaObject);

        if (SUCCESS != rc) {
                IOT_ERROR("Shadow Register Delta Error");
        }

        // Buttons initialization
        wiringPiSetup();

        for (size_t i = 0; i < TOTAL_BUTTONS; i++) {

                // Set pin to output in case it's not
                pinMode(buttonPinArray[i], INPUT);
                pullUpDnControl(buttonPinArray[i], PUD_UP);

                // Time now
                gettimeofday(&lastChange[i], NULL);

                // Bind to interrupt
                wiringPiISR(buttonPinArray[i], INT_EDGE_FALLING, buttonCallback[i]);
                //Enable button inputs
                buttonEnabledArray[i]=1;
        }

        uint8_t buttonEnabledMasKInt = buttonEnabledMask();;
        jsonStruct_t buttonEnabledHandler;
        buttonEnabledHandler.pKey = "buttonMask";
        buttonEnabledHandler.pData = &buttonEnabledMasKInt;
        buttonEnabledHandler.dataLength = sizeof(uint8_t);
        buttonEnabledHandler.type = SHADOW_JSON_INT8;

        while (NETWORK_ATTEMPTING_RECONNECT == rc || NETWORK_RECONNECTED == rc ||
               SUCCESS == rc) {
                rc = aws_iot_shadow_yield(&mqttClient, 10);
                if (NETWORK_ATTEMPTING_RECONNECT == rc) {
                        sleep(1);
                        // If the client is attempting to reconnect we will skip the rest of the
                        // loop.
                        return rc;
                }
                if (messageArrivedOnDelta) {
                        IOT_INFO("\nSending delta message back %s\n", stringToEchoDelta);
                        rc = aws_iot_shadow_update(&mqttClient, AWS_IOT_MY_THING_NAME,
                                                   stringToEchoDelta, UpdateStatusCallback, NULL,
                                                   2, true);
                        messageArrivedOnDelta = false;
                }

                if (buttonInterruptCheck()) {
                        buttonPressed = buttonPressedMask();
                        buttonHandler.pData = &buttonPressed;
                        cleanButtonInterrupts();
                        IOT_INFO(
                                "\n==============================================================="
                                "========================\n");

                        rc = aws_iot_shadow_init_json_document(JsonDocumentBuffer,
                                                               sizeOfJsonDocumentBuffer);
                        if (SUCCESS == rc) {
                                rc = aws_iot_shadow_add_reported(
                                        JsonDocumentBuffer, sizeOfJsonDocumentBuffer, 2, &buttonHandler,&buttonEnabledHandler);
                                if (SUCCESS == rc) {
                                        rc = aws_iot_finalize_json_document(JsonDocumentBuffer,
                                                                            sizeOfJsonDocumentBuffer);
                                        if (SUCCESS == rc) {
                                                IOT_INFO("Update Shadow: %s", JsonDocumentBuffer);
                                                rc = aws_iot_shadow_update(&mqttClient, AWS_IOT_MY_THING_NAME,
                                                                           JsonDocumentBuffer, UpdateStatusCallback,
                                                                           NULL, 4, true);
                                        }
                                }
                        }
                        IOT_INFO(
                                "*****************************************************************"
                                "************************\n");
                }
        }
        if (SUCCESS != rc) {
                IOT_ERROR("An error occurred in the loop %d", rc);
        }

        IOT_INFO("Disconnecting");
        rc = aws_iot_shadow_disconnect(&mqttClient);

        if (SUCCESS != rc) {
                IOT_ERROR("Disconnect error %d", rc);
        }

        return rc;
}
void buttonCallbackHandler(int button) {
        struct timeval now;
        unsigned long diff;

        gettimeofday(&now, NULL);

        // Time difference in usec
        diff = (now.tv_sec * 1000000 + now.tv_usec) -
               (lastChange[button].tv_sec * 1000000 + lastChange[button].tv_usec);

        // Filter jitter
        if (diff > IGNORE_CHANGE_BELOW_USEC) {
                printf("Falling Edge Button %d\n",button);
                buttonTriggers[button] = true;
                lastChange[button] = now;
        }


}

/**
 * @brief This function builds a full Shadow expected JSON document by putting
 * the data in the reported section
 *
 * @param pJsonDocument Buffer to be filled up with the JSON data
 * @param maxSizeOfJsonDocument maximum size of the buffer that could be used to
 * fill
 * @param pReceivedDeltaData This is the data that will be embedded in the
 * reported section of the JSON document
 * @param lengthDelta Length of the data
 */
bool buildJSONForReported(char *pJsonDocument, size_t maxSizeOfJsonDocument,
                          const char *pReceivedDeltaData,
                          uint32_t lengthDelta) {
        int32_t ret;

        if (NULL == pJsonDocument) {
                return false;
        }

        char tempClientTokenBuffer[MAX_SIZE_CLIENT_TOKEN_CLIENT_SEQUENCE];

        if (aws_iot_fill_with_client_token(tempClientTokenBuffer,
                                           MAX_SIZE_CLIENT_TOKEN_CLIENT_SEQUENCE) !=
            SUCCESS) {
                return false;
        }

        ret = snprintf(pJsonDocument, maxSizeOfJsonDocument,
                       "{\"state\":{\"desired\":null,\"reported\":%.*s}, \"clientToken\":\"%s\"}",
                       lengthDelta, pReceivedDeltaData, tempClientTokenBuffer);

        if (ret >= maxSizeOfJsonDocument || ret < 0) {
                return false;
        }

        return true;
}

void parseInputArgsForConnectParams(int argc, char **argv) {
        int opt;

        while (-1 != (opt = getopt(argc, argv, "h:p:c:"))) {
                switch (opt) {
                case 'h':
                        strncpy(HostAddress, optarg, HOST_ADDRESS_SIZE);
                        IOT_DEBUG("Host %s", optarg);
                        break;
                case 'p':
                        port = atoi(optarg);
                        IOT_DEBUG("arg %s", optarg);
                        break;
                case 'c':
                        strncpy(certDirectory, optarg, PATH_MAX + 1);
                        IOT_DEBUG("cert root directory %s", optarg);
                        break;
                case '?':
                        if (optopt == 'c') {
                                IOT_ERROR("Option -%c requires an argument.", optopt);
                        } else if (isprint(optopt)) {
                                IOT_WARN("Unknown option `-%c'.", optopt);
                        } else {
                                IOT_WARN("Unknown option character `\\x%x'.", optopt);
                        }
                        break;
                default:
                        IOT_ERROR("ERROR in command line argument parsing");
                        break;
                }
        }
}

void DeltaCallback(const char *pJsonValueBuffer, uint32_t valueLength,
                   jsonStruct_t *pJsonStruct_t) {
        IOT_UNUSED(pJsonStruct_t);
        system("amixer --card 1 sset PCM  80%,40%");
        system("aplay  success-notification-alert_A_major.wav");

        IOT_DEBUG("Received Delta message %.*s", valueLength, pJsonValueBuffer);
        //IoT_Error_t parseStringValue(char *buf, size_t bufLen, const char *jsonString, jsmntok_t *token);
        jsmn_init(&jsonParser);
        tokenCount = jsmn_parse(&jsonParser,pJsonValueBuffer, valueLength,
                                jsonTokenStruct, MAX_JSON_TOKEN_EXPECTED);
        if(tokenCount < 0) {
                IOT_WARN("Failed to parse JSON: %d", tokenCount);
                return;
        }
        /* Assume the top-level element is an object */
        if(tokenCount < 1 || jsonTokenStruct[0].type != JSMN_OBJECT) {
                IOT_WARN("Top Level is not an object");
                return;
        }
        IOT_WARN("Total token counts: %d", tokenCount);
        IOT_WARN("token struct: %.*s", jsonTokenStruct[0].type);


        jsmntok_t *tokButtonMask;
        uint16_t buttonMask;
        tokButtonMask = findToken("buttonMask", pJsonValueBuffer, jsonTokenStruct);
        IoT_Error_t rc;
        if(tokButtonMask) {
                rc =parseUnsignedInteger16Value(&buttonMask,pJsonValueBuffer,tokButtonMask);
                if(SUCCESS != rc) {
                        IOT_ERROR("parseStringValue returned error : %d ", rc);
                        return;
                }else{
                        for (size_t i = 0; i < TOTAL_BUTTONS; i++) {
                                buttonEnabledArray[i]=buttonMask&(1<<i);
                                IOT_DEBUG("buttonMask %d i: %d bool: %d",buttonMask,i,buttonMask&(1<<i));
                                /* code */
                        }
                }
        }
        if (buildJSONForReported(stringToEchoDelta, SHADOW_MAX_SIZE_OF_RX_BUFFER,
                                 pJsonValueBuffer, valueLength)) {
                messageArrivedOnDelta = true;
        }
        cleanButtonInterrupts();
}

void UpdateStatusCallback(const char *pThingName, ShadowActions_t action,
                          Shadow_Ack_Status_t status,
                          const char *pReceivedJsonDocument,
                          void *pContextData) {
        IOT_UNUSED(pThingName);
        IOT_UNUSED(action);
        IOT_UNUSED(pReceivedJsonDocument);
        IOT_UNUSED(pContextData);

        if (SHADOW_ACK_TIMEOUT == status) {
                IOT_INFO("Update Timeout--");
        } else if (SHADOW_ACK_REJECTED == status) {
                IOT_INFO("Update RejectedXX");
        } else if (SHADOW_ACK_ACCEPTED == status) {
                IOT_INFO("Update Accepted !!");
        }
}
bool buttonInterruptCheck(void){
        for (size_t i = 0; i < TOTAL_BUTTONS; i++) {
                if ((buttonTriggers[i])&&(buttonEnabledArray[i])) {
                        IOT_INFO("Button: %d Pressed",i);
                        return true;
                }
        }
        return false;
}
void cleanButtonInterrupts(void){
        for (size_t i = 0; i < TOTAL_BUTTONS; i++) {
                buttonTriggers[i]=0;
        }
}
int buttonPressedMask(void){
        uint8_t buttonMask=0;
        for (size_t i = 0; i < TOTAL_BUTTONS; i++) {
                buttonMask=buttonMask|((uint8_t)buttonTriggers[i]<<i);
        }
        return buttonMask;
}
int buttonEnabledMask(void){
        uint8_t buttonMask=0;
        for (size_t i = 0; i < TOTAL_BUTTONS; i++) {
                buttonMask=buttonMask|((uint8_t)buttonEnabledArray[i]<<i);
        }
        return buttonMask;
}
