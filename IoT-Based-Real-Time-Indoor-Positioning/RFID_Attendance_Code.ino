#include <SPI.h>
#include <MFRC522.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecureBearSSL.h>

//-----------------------------------------
// Define RFID Reader Pins
#define RST_PIN D3 // Reset pin for the MFRC522 RFID module
#define SS_PIN D4  // Slave Select (SS) pin for the MFRC522 RFID module
#define BUZZER D2  // Pin connected to the Buzzer for audio feedback
#define LED D1     // Pin connected to the LED for visual feedback
//-----------------------------------------
// Create RFID Reader object
MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
MFRC522::StatusCode status;

//-----------------------------------------
// Define RFID Block and Buffer parameters
int blockNum = 2;
byte bufferLen = 18;
byte readBlockData[18];
//-----------------------------------------
// Initialize variables for card holder name and Google Sheets API URL
String card_holder_name;
const String sheet_url = "https://script.google.com/macros/s/AKfycbxlZSAKH2zoyj55nD9k0TEdQXM3vuZtD9DhlaMRVcIb-6DL_fkWqaYXfg8IdmYAlBxN-g/exec?name=";
//-----------------------------------------
// Define the fingerprint for secure WiFi connection
const uint8_t fingerprint[20] = {0xbb, 0xb9, 0x27, 0xfb, 0x7d, 0xf3, 0xa7, 0x1a, 0x57, 0xcc, 0x23, 0xf8,
                                 0x42, 0xe9, 0x10, 0xbe, 0x59, 0x7e, 0x1f, 0xd4};
//-----------------------------------------
// Define WiFi credentials
#define WIFI_SSID "saarthak"
#define WIFI_PASSWORD "sarkar11"
//-----------------------------------------

void setup()
{
    // Initialize serial communication
    Serial.begin(9600);

    // Connect to WiFi
    Serial.println("Connecting to WiFi");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(200);
    }
    Serial.println("WiFi connected. IP address: " + WiFi.localIP().toString());

    // Set up pins for BUZZER and LED
    pinMode(BUZZER, OUTPUT);
    pinMode(LED, OUTPUT);

    // Initialize SPI bus
    SPI.begin();
}

void loop()
{
    // Check WiFi connection status
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WiFi not connected. Exiting loop.");
        return;
    }

    // Initialize RFID module
    mfrc522.PCD_Init();

    // Check for a new RFID card
    if (!mfrc522.PICC_IsNewCardPresent())
    {
        return;
    }

    // Read RFID card serial
    if (!mfrc522.PICC_ReadCardSerial())
    {
        return;
    }

    // Read data from RFID card block
    Serial.println();
    Serial.println("Reading last data from RFID...");
    ReadDataFromBlock(blockNum, readBlockData);

    // Print the data read from RFID card
    Serial.println();
    Serial.print("Last data in RFID:");
    Serial.print(blockNum);
    Serial.print(" --> ");
    for (int j = 0; j < 16; j++)
    {
        Serial.write(readBlockData[j]);
    }
    Serial.println();

    // Activate BUZZER and LED for feedback
    digitalWrite(BUZZER, LOW);
    digitalWrite(LED, HIGH);
    delay(200);
    digitalWrite(BUZZER, HIGH);
    digitalWrite(LED, LOW);
    delay(400);
    digitalWrite(BUZZER, LOW);
    digitalWrite(LED, HIGH);
    delay(200);
    digitalWrite(BUZZER, HIGH);
    digitalWrite(LED, LOW);

    // Check if WiFi is still connected
    if (WiFi.status() == WL_CONNECTED)
    {
        // Create secure WiFi client
        std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
        client->setFingerprint(fingerprint);

        // Build Google Sheets API URL with card holder name
        card_holder_name = sheet_url + String((char *)readBlockData);
        card_holder_name.trim();
        Serial.println(card_holder_name);

        // Send HTTP GET request to Google Sheets API
        HTTPClient https;
        Serial.print("[HTTPS] begin...\n");

        if (https.begin(*client, card_holder_name))
        {
            Serial.print("[HTTPS] GET...\n");
            int httpCode = https.GET();

            if (httpCode > 0)
            {
                Serial.printf("[HTTPS] GET... code: %d\n", httpCode);
                digitalWrite(BUZZER, LOW);
                digitalWrite(LED, HIGH);
                delay(1000);
                digitalWrite(BUZZER, HIGH);
                digitalWrite(LED, LOW);
            }
            else
            {
                Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
            }

            https.end();
            delay(1000);
        }
        else
        {
            Serial.printf("[HTTPS] Unable to connect\n");
        }
    }
}

void ReadDataFromBlock(int blockNum, byte readBlockData[])
{
    // Initialize key for authentication
    for (byte i = 0; i < 6; i++)
    {
        key.keyByte[i] = 0xFF;
    }

    // Authenticate the RFID card for read access using Key A
    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));

    if (status != MFRC522::STATUS_OK)
    {
        Serial.print("Authentication failed for Read: ");
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
    }
    else
    {
        Serial.println("Authentication success");
    }

    // Read data from the RFID card block
    status = mfrc522.MIFARE_Read(blockNum, readBlockData, &bufferLen);

    if (status != MFRC522::STATUS_OK)
    {
        Serial.print("Reading failed: ");
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
    }
    else
    {
        Serial.println("Block was read successfully");
    }
}
