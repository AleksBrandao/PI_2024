
#ifndef CONFIG_H
#define CONFIG_H

#include <IPAddress.h>

// esp 32-cam Endereço MAC: 24:DC:C3:AC:AD:FC
// esp32 Endereço MAC: EC:64:C9:85:AE:B4
// nova esp 32-cam Endereço MAC: 08:f9:e0:c6:a9:68


// Estrutura para armazenar o endereço MAC do parceiro

//uint8_t partnerMacAddress[] = {0x08, 0xf9, 0xe0, 0xc6, 0xa9, 0x68};
uint8_t partnerMacAddress[] = {0x24, 0xdc, 0xc3, 0xac, 0xad, 0xfc};

//ESP01 MAC: 8c:aa:b5:7c:16:62
//MAC: 8c:aa:b5:7c:16:62

uint8_t partnerMacAddress_ESP01[] = {0x8c, 0xaa, 0xb5, 0x7c, 0x16, 0x62};

// IPAddress local_IP(10, 0, 0, 254);
// IPAddress gateway(10, 0, 0, 1);
//
IPAddress local_IP(192, 168, 15, 254);
IPAddress gateway(192, 168, 15, 1);
IPAddress subnet(255, 255, 0, 0);


const char *ssid = "VIVOFIBRA-5221";
const char *password = "kPcsBo9tdC";

// const char *ssid = "INTELBRAS";
// const char *password = "Anaenena";

// Insert Firebase project API Key
#define API_KEY "4ed938647d58af6ebe78e9854feae22da020c64e"
// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://reconhecimento-facial-cbae7-default-rtdb.firebaseio.com"
#define USER_EMAIL "aleks.brandao@gmail.com"
#define USER_PASSWORD "reconhecimento"

#endif // CONFIG_H
