//example 2 level delta function
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

        jsmntok_t *tokReported;
        tokReported = findToken("buttonPressed", pJsonValueBuffer, jsonTokenStruct);
        if (tokReported) {
                IOT_INFO("execution: %.*s", tokReported->end - tokReported->start,
                         (char *)pJsonValueBuffer + tokReported->start);
                IoT_Error_t rc;
                char stringBuffer[SHADOW_MAX_SIZE_OF_RX_BUFFER];
                rc = parseStringValue(stringBuffer, SHADOW_MAX_SIZE_OF_RX_BUFFER + 1,
                                      pJsonValueBuffer, tokButton);
                if(SUCCESS != rc) {
                        IOT_ERROR("parseStringValue returned error : %d ", rc);
                        return;
                }else{
                        IOT_DEBUG("String Buffer %s",stringBuffer);
                }
                //bool b=stringBuffer=="true";
                jsmntok_t *tokButton;
                tokButton = findToken("buttonPressed", pJsonValueBuffer, tokReported);
                if (tokButton) {
                        IoT_Error_t rc;
                        char stringBuffer[SHADOW_MAX_SIZE_OF_RX_BUFFER];
                        rc = parseStringValue(stringBuffer, SHADOW_MAX_SIZE_OF_RX_BUFFER + 1,
                                              pJsonValueBuffer, tokButton);
                        if(SUCCESS != rc) {
                                IOT_ERROR("parseStringValue returned error : %d ", rc);
                                return;
                        }else{
                                IOT_DEBUG("String Buffer %s",stringBuffer);
                        }

                }else{
                        IOT_DEBUG("Invalid tokButton");
                }
        }else{
                IOT_DEBUG("Invalid tokReported");
        }
        /*
           char stringBuffer[SHADOW_MAX_SIZE_OF_RX_BUFFER];
           IoT_Error_t error=parseStringValue(stringBuffer, SHADOW_MAX_SIZE_OF_RX_BUFFER, stringToEchoDelta, jsmnTokens);
           IOT_DEBUG("String Buffer %s",stringBuffer);
           if (buildJSONForReported(stringToEchoDelta, SHADOW_MAX_SIZE_OF_RX_BUFFER,
                                 pJsonValueBuffer, valueLength)) {
                messageArrivedOnDelta = true;
           }else{
                IOT_DEBUG("Error building JSON");
           }
         */
}
/*
   //jsmntok_t *tokReported;
   //tokReported = findToken("buttonPressed", pJsonValueBuffer, jsonTokenStruct);
   //IOT_WARN("token reported type: %d", tokReported->type );
   if (tokReported) {
        //IOT_INFO("button: %.*s", tokReported->end - tokReported->start,
        //         (char *)pJsonValueBuffer + tokReported->start);
        //char stringBuffer[SHADOW_MAX_SIZE_OF_RX_BUFFER +1];
        //snprintf(stringBuffer, 100, "%.*s", tokReported->end - tokReported->start, (char *)pJsonValueBuffer + tokReported->start);
        //IOT_INFO("string: %s", stringBuffer);

        bool buttonValue;
        IoT_Error_t rc;
        //value can have 4 types included in aws_json_utils.c
        rc =parseUnsignedInteger16Value(&buttonMask,pJsonValueBuffer,tokButtonMask);
        rc = parseBooleanValue(&buttonValue,pJsonValueBuffer, tokReported);

        if(SUCCESS != rc) {
                IOT_ERROR("parseStringValue returned error : %d ", rc);
                IOT_DEBUG("buttonValue %s", buttonValue ? "true" : "false");
                return;
        }else{
                IOT_DEBUG("buttonValue %s", buttonValue ? "true" : "false");
        }

           if (stringBuffer != NULL) {
                IOT_INFO("Delta - Button state changed to %d", *(bool *)(stringBuffer));
           }else{
                IOT_INFO("String value: %s", stringBuffer);
           }


        //bool b=stringBuffer=="true";
   }else{
        IOT_DEBUG("Invalid tokReported");
   }
 */
