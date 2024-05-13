

// esp32 em 2.0.15
// Firebase_ESP_Client em 2.3.7
//placa ESP32 Dev Module

#include "config.h"
#include <Arduino.h>
#include <FS.h> //isso precisa ser o primeiro, ou tudo trava e queima...
#include "SPIFFS.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPI.h>
#include <MFRC522.h>
#include <vector>
#include "time.h" //<<< sheets
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <esp_now.h>
#include <Firebase_ESP_Client.h>
// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"
// Definição do nome do dispositivo
#define DEVICE_NAME "ESP32CAM"
// Insert Firebase project API Key
#define API_KEY "AIzaSyAJn68X4FRmxdk8NMu0ir9LwRsrIr7j7F0"
// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://reconhecimento-facial-cbae7-default-rtdb.firebaseio.com"
#define USER_EMAIL "aleks.brandao@gmail.com"
#define USER_PASSWORD "reconhecimento"

String usuarioEncontrado; // Variável global para armazenar o usuário encontrado
String usuarioCartao; // Variável global para armazenar o userID

// Define Firebase Data object
FirebaseData fbdo;
String uidTag;
String dateTime; // Altere para String em vez de const char*
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
int loopCount = 0;
bool signupOK = false;

#define FILENAME "/Cadastro.txt"
#define LED_RED 15  // D3
#define LED_GREEN 2 // D4
#define SDA_PIN 21  // ok
#define SCK_PIN 18  // ok
#define MOSI_PIN 23 // ok
#define MISO_PIN 19 // ok
#define RST_PIN 5   //

using namespace std;

WiFiClient espClient;
PubSubClient client(espClient);

String info_data; // Informação sobre o usuario.Ex nome, cpf, etc.
String id_data;   // Id para o usuario.
int index_user_for_removal = -1;

String rfid_card = ""; // Codigo RFID obtido pelo Leitor
String sucess_msg = "";
String failure_msg = "";
MFRC522 mfrc522(SDA_PIN, RST_PIN);
// Cria um objeto AsyncWebServer que usará a porta 80
AsyncWebServer server(80);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
unsigned long lastTime = 0;
unsigned long timerDelay = 500; // Intervalo de tempo em milissegundos entre os envios para a planilha (30 segundos)

// NTP server to request epoch time
const char *ntpServer = "time.google.com"; // Servidor NTP para obter o tempo em formato epoch
const long gmtOffset_sec = -3 * 3600;      // -10800 segundos
const int daylightOffset_sec = 0;          // Offset para horário de verão, se necessário
unsigned long epochTime;                   // Variável para armazenar o tempo atual em formato epoch

unsigned long getTime()
{
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    // Serial.println("Failed to obtain time");
    return (0);
  }
  time(&now);
  return now; // Retorna o tempo atual em formato epoch
}


void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}
// Inicializa o sistema de arquivos.
bool initFS()
{
  if (!SPIFFS.begin(true))
  {
    Serial.println("Erro ao abrir o sistema de arquivos");
    return false;
  }
  Serial.println("Sistema de arquivos carregado com sucesso!");
  return true;
}
// Lista todos os arquivos salvos na flash.
void listAllFiles()
{
  Serial.println("###################### - inicio- ############################");
  String str = "";
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while (file)
  {
    str += file.name();
    str += " / ";
    str += file.size();
    str += "\r\n";
    file = root.openNextFile();
  }
  Serial.print(str);
  Serial.println("###################### - fim - ############################");
}

// Faça a leitura de um arquivo e retorne um vetor com todas as linhas.
vector<String> readFile(String path)
{
  vector<String> file_lines;
  String content;
  File myFile = SPIFFS.open(path.c_str(), "r");
  if (!myFile)
  {
    myFile.close();
    return {};
  }
//  Serial.println("###################### - FILE- ############################");
  while (myFile.available())
  {
    content = myFile.readStringUntil('\n');
    file_lines.push_back(content);
//    Serial.println(content);
  }
//  Serial.println("###########################################################");
  myFile.close();
  return file_lines;
}
// Faça a busca de um usuario pelo ID e pela INFO.
int findUser(vector<String> users_data, String id, String info)
{
  String newID = "<td>" + id + "</td>";
  String newinfo = "<td>" + info + "</td>";

  for (int i = 0; i < users_data.size(); i++)
  {
    if (users_data[i].indexOf(newID) > 0 || users_data[i].indexOf(newinfo) > 0)
    {
      return i;
    }
  }
  return -1;
}
// Adiciona um novo usuario ao sistema
bool addNewUser(String id, String data)
{
  File myFile = SPIFFS.open(FILENAME, "a+");
  if (!myFile)
  {
    Serial.println("Erro ao abrir arquivo!");
    myFile.close();
    return false;
  }
  else
  {
    myFile.printf("<tr><td>%s</td><td>%s</td>\n", id.c_str(), data.c_str());
    Serial.println("Arquivo gravado");
  }
  myFile.close();
  return true;
}
// Remove um usuario do sistema
bool removeUser(int user_index)
{
  vector<String> users_data = readFile(FILENAME);
  if (user_index == -1) // Caso usuário não exista retorne falso
    return false;

  File myFile = SPIFFS.open(FILENAME, "w");
  if (!myFile)
  {
    Serial.println("Erro ao abrir arquivo!");
    myFile.close();
    return false;
  }
  else
{
  Serial.println("Iniciando remoção de usuário...");
for (int i = 0; i < users_data.size(); i++) {
  if (i == user_index) {
    Serial.println("Usuário a ser removido: " + users_data[i]);
  } else {
    myFile.println(users_data[i]);
    Serial.println("Escrevendo no arquivo: " + users_data[i]);
  }
}
}


  myFile.close();
  return true;
}
// Esta função substitui trechos de paginas html marcadas entre %
String processor(const String &var)
{
  String msg = "";
  if (var == "TABLE")
  {
    msg = "<table><tr><td>RFID Code</td><td>User Info</td><td>Delete</td></tr>";
    vector<String> lines = readFile(FILENAME);
    for (int i = 0; i < lines.size(); i++)
    {
      msg += lines[i];
      msg += "<td><a href=\"get?remove=" + String(i + 1) + "\"><button>Excluir</button></a></td></tr>"; // Adiciona um botão com um link para o indice do usuário na tabela
    }
    msg += "</table>";
  }
  else if (var == "SUCESS_MSG")
    msg = sucess_msg;
  else if (var == "FAILURE_MSG")
    msg = failure_msg;
  return msg;
}

// esp_now_peer_info_t peerInfo;
 void sendEspNowCommand(const char* command) {
    esp_err_t result = esp_now_send(partnerMacAddress, (uint8_t*)command, strlen(command) + 1);
    if (result == ESP_OK) {
        Serial.println("Confirmação de mensagem enviada com sucesso.");
    } else {
        Serial.println("Falha ao enviar mensagem de confirmação.");
    }
}

void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len)
{
  Serial.print("Mensagem ESPNOW recebida de ");
  Serial.print(DEVICE_NAME);
  Serial.print(": ");
  Serial.println(String((char*)data));
  sendEspNowCommand("Confirmação que a mensagem foi recebida");


  // Convertendo os dados recebidos para uma string para fácil manipulação
  String input = String((char*)data);
delay(100);
  // Encontrando a posição do ':'
  int pos = input.indexOf(':');
delay(100);
  if (pos != -1) {
    // Separando a mensagem em "command" e "userID"
    String command = input.substring(0, pos);
    String usuarioEncontrado = input.substring(pos + 1); // Pulando o ':' e o espaço
//if (command != "Reconhecido") {
//   Serial.print("Passou neste if");
//    }
    Serial.print("Command: ");
    Serial.println(command);
    Serial.print("Usuario reconhecido: ");
    Serial.println(usuarioEncontrado);
    Serial.print("Usuario recebido da ESP32: ");
    Serial.println(usuarioCartao);

// Se você também deseja remover espaços internos
usuarioCartao.replace(" ", "");
usuarioEncontrado.replace(" ", "");

//Serial.println(":" + usuarioCartao);
//Serial.println(":" + usuarioEncontrado);

   if (usuarioCartao == usuarioEncontrado) {
    Serial.println("Usuário reconhecido corresponde ao registrado");
    const char* message = "open";
    esp_err_t result = esp_now_send(partnerMacAddress_ESP01, (uint8_t *)message, strlen(message) + 1); // Envia a mensagem para abrir a fechadura

    if (result == ESP_OK)
      {
        Serial.println("Mensagem para ESP01 enviada com sucesso");
      }
      else
      {
        Serial.println("Erro ao enviar a mensagem para ESP01");
      }
} else {
    Serial.println("Usuário não reconhecido");
}
    }
}

esp_now_peer_info_t peerInfo;

void setup()
{
  Serial.begin(115200);

  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);

  SPI.begin();
  mfrc522.PCD_Init(); // Inicia MFRC522

  // Inicialize o SPIFFS
  if (!initFS())
    return;
  listAllFiles();
  readFile(FILENAME);

  // Conectando ao Wi-Fi
//  WiFi.begin(ssid, password);
//  while (WiFi.status() != WL_CONNECTED)
//  {
//    delay(1000);
//    Serial.println("Conectando WiFi..");
//    // Configura o fuso horário (GMT-3:00 para São Paulo, Brasil)
//    //  setTimeOffset(-3 * 3600);
//    timeClient.begin();
//  }

 if (!WiFi.config(local_IP, gateway, subnet))
  {
    Serial.println("STA Failed to configure");
  }
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  Serial.println(WiFi.localIP());

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  // auth.user.email = USER_EMAIL;
  // auth.user.password = USER_PASSWORD;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", ""))
  {
    Serial.println("ok");
    signupOK = true;
  }
  else
  {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // client.setServer(mqtt_server, mqtt_port);
  // client.setCallback(callback);

  // Inicializar o ESP-NOW
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Erro ao inicializar o ESP-NOW");
    return;
  }

  // Configurar a função de callback para receber mensagens
  esp_now_register_recv_cb(OnDataRecv);

  // Registrar o parceiro
  // esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, partnerMacAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("Falha ao adicionar o primeiro parceiro");
    return;
  }
  // Se chegou aqui, significa que o parceiro foi adicionado com sucesso.
  Serial.println("Primeiro parceiro adicionado com sucesso!");

   memcpy(peerInfo.peer_addr, partnerMacAddress_ESP01, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("Falha ao adicionar o segundo parceiro (ESP-01)");
    return;
  }
  // Se chegou aqui, significa que o parceiro foi adicionado com sucesso.
  Serial.println("Segundo parceiro (ESP-01) adicionado com sucesso!");

  // Rotas.
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            {
        rfid_card = "";
        request->send(SPIFFS, "/home.html", String(), false, processor); });
  server.on("/home", HTTP_GET, [](AsyncWebServerRequest *request)
            {
        rfid_card = "";
        request->send(SPIFFS, "/home.html", String(), false, processor); });
  server.on("/sucess", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/sucess.html", String(), false, processor); });
  server.on("/warning", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/warning.html"); });
  server.on("/failure", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/failure.html"); });
  server.on("/deleteuser", HTTP_GET, [](AsyncWebServerRequest *request)
            {
        if (removeUser(index_user_for_removal)) {
          sucess_msg = "Usuário excluido do registro.";
          request->send(SPIFFS, "/sucess.html", String(), false, processor);
        }
        else
          request->send(SPIFFS, "/failure.html", String(), false, processor);
        return; });
  server.on("/stylesheet.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/stylesheet.css", "text/css"); });
  server.on("/rfid", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/plain", rfid_card.c_str()); });
  server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request)
            {
        vector <String> users_data = readFile(FILENAME);
    
        if (request->hasParam("info")) {
          info_data = request->getParam("info")->value();
          info_data.toUpperCase();
          Serial.printf("info: %s\n", info_data.c_str());
        }
        if (request->hasParam("rfid")) {
          id_data = request->getParam("rfid")->value();
          Serial.printf("ID: %s\n", id_data.c_str());
        }
        if (request->hasParam("remove")) {
          String user_removed = request->getParam("remove")->value();
          Serial.printf("Remover o usuário da posição : %s\n", user_removed.c_str());
          index_user_for_removal = user_removed.toInt();
          index_user_for_removal -= 1;
          request->send(SPIFFS, "/warning.html");
          return;
        }
        if(id_data == "" || info_data == ""){
          failure_msg = "Informações de usuário estão incompletas.";
          request->send(SPIFFS, "/failure.html", String(), false, processor);
          return;
          }
        int user_index = findUser(users_data, id_data, info_data);
        if (user_index < 0) {
          Serial.println("Cadastrando novo usuário");
          addNewUser(id_data, info_data);
          sucess_msg = "Novo usuário cadastrado.";
          request->send(SPIFFS, "/sucess.html", String(), false, processor);
        }
        else {
          Serial.printf("Usuário numero %d ja existe no banco de dados\n", user_index);
          failure_msg = "Ja existe um usuário cadastrado.";
          request->send(SPIFFS, "/failure.html", String(), false, processor);
        } });
  server.on("/logo.jpg", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/logo.jpg", "image/jpg"); });

            server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request) {
  // Verifica se o arquivo existe
  if (SPIFFS.exists(FILENAME)) {
    // Envia o arquivo como um download
    request->send(SPIFFS, FILENAME, "text/plain", true);
  } else {
    // Se o arquivo não existir, envia uma resposta de erro 404
    request->send(404, "text/plain", "File Not Found");
  }
});


  server.onNotFound(notFound);
  // Inicia o serviço
  server.begin();

  // Configure time //<<sheets
  configTime(0, 0, ntpServer); // Configura o tempo utilizando o servidor NTP //<<sheets
}

void loop()
{

  // if (!client.connected())
  // {
  //   reconnect();
  // }
  // client.loop();

  // Procure por novos cartões.
  if (!mfrc522.PICC_IsNewCardPresent())
  {
    return;
  }
  // Faça a leitura do ID do cartão
  if (mfrc522.PICC_ReadCardSerial())
  {
//    Serial.print("UID da tag :");
    String rfid_data = "";
    for (uint8_t i = 0; i < mfrc522.uid.size; i++)
    {
      rfid_data.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
      rfid_data.concat(String(mfrc522.uid.uidByte[i], HEX));
    }
    // Serial.println();
    rfid_data.toUpperCase();
    rfid_card = rfid_data;

    // Carregue o arquivo de cadastro
    vector<String> users_data = readFile(FILENAME);
    // Faça uma busca pelo id
    int user_index = findUser(users_data, rfid_data, "");

    uidTag = rfid_data;
    // client.publish(mqtt_topic, rfid_card.c_str());

    Serial.println("uidTag = " + uidTag);

    if (user_index < 0)
    {
      Serial.printf("Nenhum usuário encontrado\n");
      digitalWrite(LED_RED, HIGH);

       String command = "capture";
       String userID = uidTag; // Substitua "123" pela string do ID do usuário real
       String message = command + ":" + userID;
       usuarioCartao = userID;

      esp_err_t result = esp_now_send(partnerMacAddress, (uint8_t *)message.c_str(), message.length() + 1); // +1 para incluir o caractere nulo no final
      if (result == ESP_OK)
      {
        Serial.println("Mensagem ESPNOW enviada com sucesso");
      }
      else
      {
        Serial.println("Erro ao enviar a mensagem");
      }

    }
    else
    {
      Serial.printf("Usuário %d encontrado\n", user_index);
      digitalWrite(LED_GREEN, HIGH);

       String command = "start_recognition";
       String userID = uidTag; // Substitua "123" pela string do ID do usuário real
       String message = command + ":" + userID;
       usuarioCartao = userID;

      esp_err_t result = esp_now_send(partnerMacAddress, (uint8_t *)message.c_str(), message.length() + 1); // +1 para incluir o caractere nulo no final
      if (result == ESP_OK)
      {
        Serial.println("Mensagem ESPNOW enviada com sucesso");
      }
      else
      {
        Serial.println("Erro ao enviar a mensagem");
     
      }

    }
    delay(1000);
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, LOW);

    if (Firebase.ready() && signupOK)
    {
        updateDateTime(); // Atualiza a variável dateTime

     FirebaseJson json;
     json.set("id", uidTag);
     json.set("data", dateTime);
     json.set("device", "RFID");

    String path = "entrys/";
  path += String(millis()); // Usando o timestamp como ID único

  if (Firebase.RTDB.setJSON(&fbdo, path.c_str(), &json)) {
    Serial.println("Dados enviados com sucesso para Firebase!");
  } else {
    Serial.println("Falha no envio de dados: " + fbdo.errorReason());
  }
      
    }
    else
    {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  }
}

void updateDateTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    dateTime = "Unavailable"; // Atribui um valor padrão se falhar
    return;
  }

  char buffer[30]; // Buffer para armazenar a data e hora formatadas
  strftime(buffer, sizeof(buffer), "%d/%m/%Y %H:%M:%S", &timeinfo);
  dateTime = String(buffer); // Converte para String e armazena
}
