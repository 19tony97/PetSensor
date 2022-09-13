#include "arduino_secrets.h"
#include "thingProperties.h"
#include <DHT.h>
#include <WiFiClientSecure.h>
#include <mbedtls/aes.h>

const char* ssid     = "Vodafone-34830521"; 
const char* password = "azpk6v9tp2wmkea";
const char* server = "192.168.1.5";
const int port = 8090;
char* key = "abcdefghijklmnop";

char plainTemperatureText[16];
char plainLightText[16];

const char* test_root_ca = "-----BEGIN CERTIFICATE-----\n"\
"MIIDvTCCAqWgAwIBAgIUI4lm6Ak3Tfkt44OsLeqkRToKHUEwDQYJKoZIhvcNAQEL\n"\
"BQAwbjELMAkGA1UEBhMCSVQxEjAQBgNVBAgMCVNhbGVybm9pcDEMMAoGA1UECgwD\n"\
"VUZGMRcwFQYDVQQDDA4xNzIuMTkuMTk0LjE1ODEkMCIGCSqGSIb3DQEJARYVdG9u\n"\
"eW1pbGFuOTdAZ21haWwuY29tMB4XDTIyMDkxMDAxMzUxOVoXDTIzMDkxMDAxMzUx\n"\
"OVowbjELMAkGA1UEBhMCSVQxEjAQBgNVBAgMCVNhbGVybm9pcDEMMAoGA1UECgwD\n"\
"VUZGMRcwFQYDVQQDDA4xNzIuMTkuMTk0LjE1ODEkMCIGCSqGSIb3DQEJARYVdG9u\n"\
"eW1pbGFuOTdAZ21haWwuY29tMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKC\n"\
"AQEAti5LN8g5lwbRyYBr7VX2xd8fEu7NVqN1oIeVu456bWr/0iQWu9RAHOB9Y/DR\n"\
"WNka53uBCXxDaCV7VS0EVXx2dW36xPwEfvtzuXwCjMOEUguMEHzOFbDrrDhvYk5N\n"\
"B9Huz+gcIMRCzH6KsWzPZmQpC8xSVj7RIHExmh4YXM0xq7wodjA6/7tb673SX+Ob\n"\
"NpFAtL6qLJ7qKdqYu0BuXCweXMoTUrxZqde5X74eqS1lMIg4wACe7Zu7zxmN4yxm\n"\
"smlu7PxzZDI3rMt8/GTeO2uuOgssC2QjibpqTF8tzL5h3SfDHEOMRwmWRKw6D1fn\n"\
"EhDrImHDI338Puc6ZYbh+2lFMQIDAQABo1MwUTAdBgNVHQ4EFgQUESNcUJSYqak0\n"\
"dOKty4BAoDTeItYwHwYDVR0jBBgwFoAUESNcUJSYqak0dOKty4BAoDTeItYwDwYD\n"\
"VR0TAQH/BAUwAwEB/zANBgkqhkiG9w0BAQsFAAOCAQEAMPr14o4J20TDZ/QKHYmk\n"\
"JrkB3/9Y5yA9p1kJgHP/JkNELGgcUjMf/xlm4FIJMeTtLJ6fn9j35bamAkTIU08H\n"\
"B8hQrX0SQUsw2iptz0jMwcj66taMs2EdleVuJCaRgyTL0ovq2i0JkhpkLn1twJSg\n"\
"RqbxSnd3IBjALQAlgEtVCToVD0cJNGFJ2fpVnXzDaD5f6r69RJLhBjySmHTJobY5\n"\
"SgQk39OEutqYANM33tLt0Vri5kpaeu2evZH1aQcn8Y70JpDML80qJaJzeogFoQql\n"\
"cGVQFJedhintjb+hI2E7jzIymI5rGAmNBXhN91tsVvisibhvWeREtL91IXajjNdM\n"\
"aw==\n"\
"-----END CERTIFICATE-----";


WiFiClientSecure client;

#define DHTPIN 33
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

void encrypt(char * plainText, char * key, unsigned char * outputBuffer){
  mbedtls_aes_context aes;
  mbedtls_aes_init( &aes );
  mbedtls_aes_setkey_enc( &aes, (const unsigned char*) key, strlen(key) * 8 );
  mbedtls_aes_crypt_ecb( &aes, MBEDTLS_AES_ENCRYPT, (const unsigned char*)plainText, outputBuffer);
  mbedtls_aes_free( &aes );
}

void decrypt(unsigned char * chipherText, char * key, unsigned char * outputBuffer){
  mbedtls_aes_context aes;
  mbedtls_aes_init( &aes );
  mbedtls_aes_setkey_dec( &aes, (const unsigned char*) key, strlen(key) * 8 );
  mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_DECRYPT, (const unsigned char*)chipherText, outputBuffer);
  mbedtls_aes_free( &aes );
}

void addSpaces(char* plainText){
  for(int i=0; i< (16-strlen(plainText)); i++)
    strcat(plainText," ");
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  delay(1500);
  
  Serial.print("Attempting to connect to SSID: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  // attempt to connect to Wifi network:
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    // wait 1 second for re-trying
    delay(1000);
  }
  Serial.print("Connected to ");
  Serial.println(ssid);
  
  client.setCACert(test_root_ca);
  
  Serial.println();
  
  initProperties();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();
}

void loop() {
  ArduinoCloud.update();
  unsigned char cipherTextOutput[16];
  unsigned char decipheredTextOutput[16];
  int lengthWord;
  
  temperature = dht.readTemperature();
  delay(1000);
  
  
  Serial.println(temperature);
  
  itoa(temperature, plainTemperatureText, 10);
  addSpaces(plainTemperatureText);
  
  String encryptedText = "";
  
   if (!client.connect(server, port)){
    Serial.println("Connection failed!");
  } else {
    Serial.println("Connection success!");
    
    encrypt(plainTemperatureText, key, cipherTextOutput);
   
    for (int i = 0; i < 16; i++) {
      char str[3];
      sprintf(str, "%02x", (int)cipherTextOutput[i]);
      encryptedText = encryptedText + str;
    }

    encryptedText = encryptedText + ' ';
  
    Serial.println("Mando al server "+encryptedText);
    client.println(encryptedText);
  }
  client.stop();
  delay(1000);

}
/*
  Since Temperature is READ_WRITE variable, onTemperatureChange() is
  executed every time a new value is received from IoT Cloud.
*/
void onTemperatureChange()  {
  // Add your code here to act upon Temperature change
}