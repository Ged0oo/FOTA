uint8_t dataToSend[256] = "00 50 00 20 49 8C";

uint8_t byteArray[128];
size_t dataLength = 0;

void setup() {
    Serial.begin(115200);
    removeSpaces(dataToSend);
    dataLength = parseHexData(dataToSend, byteArray);
}

void loop() 
{
    for (int i = 0; i < dataLength; i++) 
    {
        Serial.print(byteArray[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
    delay(1000);
}

void removeSpaces(uint8_t* input) 
{
    size_t j = 0;
    for (size_t i = 0; input[i] != '\0'; i++) 
    {
        if (input[i] != ' ') 
        {
            input[j++] = input[i];
        }
    }
    input[j] = '\0';
}

size_t parseHexData(uint8_t* input, uint8_t* output) 
{
    size_t dataLength = 0;
    for (int i = 0; input[i] != '\0' && input[i + 1] != '\0'; i += 2) 
    {
        char hexByte[3];
        hexByte[0] = (char)input[i];
        hexByte[1] = (char)input[i + 1];
        hexByte[2] = '\0';
        output[dataLength] = strtol(hexByte, NULL, 16);
        dataLength++;
    }
    return dataLength;
}
