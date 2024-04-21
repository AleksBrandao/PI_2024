
    // https://blog.smartkits.com.br/esp8266-cadastro-rfid-mfrc522-com-webserver/
    // https://www.arduinoecia.com.br/controle-de-acesso-modulo-rfid-rc522/ = PINAGEM RDID
    // https://chat.openai.com/c/f2c64e55-cabb-4b43-9a30-00db7d6e8aa9 CHATGPT
    
    
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
    #include <ESP_Google_Sheet_Client.h>//<<< sheets
    // For SD/SD_MMC mounting helper//<<< sheets
    #include <GS_SDHelper.h>//<<< sheets
    
    #include <NTPClient.h>
    #include <WiFiUdp.h>
    #include <TimeLib.h>
    
    //#define SS_PIN 4  //D2 SDA
    //#define RST_PIN 5 //D1 RST
    #define FILENAME "/Cadastro.txt"
    #define LED_RED 15 //D3
    #define LED_GREEN 2 //D4
    
    #define SDA_PIN 21 //ok
    #define SCK_PIN 18 //ok
    #define MOSI_PIN 23 //ok
    #define MISO_PIN 19 //ok
    #define RST_PIN 5 //
    
      
    
    
    using namespace std;
    
    // const char* ssid = "VIVOFIBRA-5221";
    // const char* password = "kPcsBo9tdC";

    const char* ssid = "INTELBRAS";
    const char* password = "Anaenena";

    // const char* mqtt_server = "broker.hivemq.com";
    const char* mqtt_server = "broker.emqx.io";
    const int mqtt_port = 1883;
    const char* mqtt_topic = "joystickData";

    WiFiClient espClient;
    PubSubClient client(espClient);

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensagem recebida [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void reconnect() {
  while (!client.connected()) {
    // Serial.print("Tentando se reconectar ao MQTT Broker...");
    if (client.connect("ESP32Client")) {
      // Serial.println("Conectado");
      client.subscribe(mqtt_topic);
    } else {
      Serial.print("Falha na conexão, rc=");
      Serial.print(client.state());
      Serial.println(" Tentando novamente em 5 segundos");
      delay(5000);
    }
  }
}

    
    WiFiUDP ntpUDP;
    NTPClient timeClient(ntpUDP, "pool.ntp.org");
    
    //<<sheets
    // Google Project ID
    #define PROJECT_ID "114777749843614379837"  // Substituir pelo ID do seu projeto do Google
    
    // Service Account's client email
    #define CLIENT_EMAIL "ab-reconhecimento@reconhecimento-418220.iam.gserviceaccount.com"  // Substituir pelo email de cliente da sua conta de serviço
    
    // Service Account's private key
    const char PRIVATE_KEY[] PROGMEM = "-----BEGIN PRIVATE KEY-----\nMIIEvAIBADANBgkqhkiG9w0BAQEFAASCBKYwggSiAgEAAoIBAQDHk1Doobik55WT\najAiMWcb1YHvAJD3cU9qemk/1ErAiuLXrDzcp/kg4r6qE1UnIHgGlMXrUiV3Eewi\nTNrIipz9ayemPgeDA92yxSoysb8cNcLh5bfEWcY+wmwizySeycD+PuNjsnrvwRGg\nNzuaDo0eEKEf6/vAhOY/R1O6XS1C7Wb/pFN/7wrfmfF5mFt//BosZDA3I53YPrbn\nPZ3SthTjLycGWfciF4Y6zCrRPFxKWZun2rekRVdg4jNvEc0G1TOgzn/+g5f3YvQp\nRP9QyvYgl2rCjgRW4L/YYRFUrfuWoTgyffj44AD5XEM+S3tlCmAVeIV1eNMja0M3\n8Cf2znNxAgMBAAECggEAAvzuHCOIDA5XEhX9NT/ZL+SfEsTcMXDh/AxBN+ZN6GAp\nSV/FPquppP/xaUK2QUprnZ6b2tF3/ovB8IQHI7OWlOE7iVzE+EWmK7YRvbX6pbma\nKroUSaVrR0yEzwhxVLVyKnmm0NBEO1jKx/ktDw9gIRbQO2tT3DlT01dLgL8XzW28\nHR4VMaUu5Y5UXmB0IlWvOStv3eZUgu9wyU/Oj6y6NqNOLD/yNRgshJ3jAOICeLNi\neQMweSMQPLGevZJurhWFP8K0mbkNHXgjcbVx3COGAzveiUlgmlAPY544Do6DL7TB\n/YzZLjpMFZPhGgxVuRMj65lYUzmZQVfZsmLsowi2QQKBgQD5Ndj4dEzBQvAkbX0Z\n47Jom4oaq7pYY2K23LL1ttclEU+GVHjHVhyO2EF0Avsy9pcf2J+eZ8Oi3/ZXtdRc\nztmR7AWnELakxEV4KvvsDEkLTUG+5bjuf1voYiU2nk1z5LgFpSHGs8Dm5QOZxhmS\nui0lZctRjYECII5HhI2ynA3P4QKBgQDNA0h0ELMByllluCy740dtnfEyeKajpjRK\nYEoB9kpvqir9tAMMzbJA+0Vb0lPlwLliBJCfxCUS8Am3V79pMRfBPt5Mpao0iiLd\nxhySy29+uQ7JmPR3Szgrp4Qnpnn93Aog798LTK6+y9BhZG0mZ8hY8WBojr+QfxRq\nJGOdk7ZVkQKBgCQwcqKZ2O+TteXEVI9m5miUdbiryXK+c/5UDFTsSU/jtKWwLJ3d\n3mXL961OJYZgEtAYGA3byagkV9Si3gTgMO4k1SlOnwdMTT5HF7BOlGjkvjBnkbRo\noEMdxYOp91tmEmcXdNEzF0cwaJZzExGgoZ+1qZHdN6fEbITsNduDF+phAoGABT75\nQqcOvZP896Jf2qr1L/PjsSPvN67QFbsjCavQuczD7twFW/WDgzAq1S+rn+xvkfeF\n7+CoBjUIOp3PMxTjg7llHNb8ZP3H6J7iKkt0XezEWRpF3yuYk11k/1K+OmXACJm6\nvmJG8nDqsyNLu7jaIpSCoApPEpZ94j1uIyEdgFECgYA/QVn56OuMYi4vDQjvfreO\n3XkN4o+F0zbwpJJpfHi5gEddfnJwJ1fN8Ccf2ho3m06BE6MweoHAlnGYWf3MBjbJ\n+ard7K/uN0E8IdbVE63KACwucf0l9a7XW/He6ZSzKHMJXq8YDRhiWAUJTBd7J9JS\n33ROANoJd8um45G+1T3HXw==\n-----END PRIVATE KEY-----\n";
    // Substituir pela chave privada da sua conta de serviço
    
    // The ID of the spreadsheet where you'll publish the data
    const char spreadsheetId[] = "1pkBYPdyPirv7kR87QNinugiXmNm_fmlBKPSBFLEbuG8";  // Substituir pelo ID da sua planilha do Google Sheets
    
    // Timer variables
    unsigned long lastTime = 0;  
    unsigned long timerDelay = 500;  // Intervalo de tempo em milissegundos entre os envios para a planilha (30 segundos)
    
    // Token Callback function
    void tokenStatusCallback(TokenInfo info);  // Função de callback para lidar com informações do token de autenticação
    
    // NTP server to request epoch time
    const char* ntpServer = "pool.ntp.org";  // Servidor NTP para obter o tempo em formato epoch
    
    // Variable to save current epoch time
    unsigned long epochTime;  // Variável para armazenar o tempo atual em formato epoch
    
    unsigned long getTime() {
      time_t now;
      struct tm timeinfo;
      if (!getLocalTime(&timeinfo)) {
        //Serial.println("Failed to obtain time");
        return(0);
      }
      time(&now);
      return now;  // Retorna o tempo atual em formato epoch
    }
    
    //<<sheets
    
    String info_data; //Informação sobre o usuario.Ex nome, cpf, etc.
    String id_data;   //Id para o usuario.
    int index_user_for_removal = -1;
    
    String rfid_card = ""; //Codigo RFID obtido pelo Leitor
    String sucess_msg = ""; 
    String failure_msg = "";
    
    // Cria um objeto  MFRC522.
    //MFRC522 mfrc522(SS_PIN, RST_PIN);   
    //MFRC522 mfrc522(SDA_PIN, RST_PIN, MOSI_PIN, MISO_PIN, SCK_PIN);
    MFRC522 mfrc522(SDA_PIN, RST_PIN);
    
    // Cria um objeto AsyncWebServer que usará a porta 80
    AsyncWebServer server(80);
    
    void notFound(AsyncWebServerRequest *request) {
      request->send(404, "text/plain", "Not found");
    }
    //Inicializa o sistema de arquivos.
    bool initFS() {
      if (!SPIFFS.begin()) {
        // Serial.println("Erro ao abrir o sistema de arquivos");
        return false;
      }
      // Serial.println("Sistema de arquivos carregado com sucesso!");
      return true;
    }
    //Lista todos os arquivos salvos na flash.
    void listAllFiles() {
      String str = "";
      File root = SPIFFS.open("/");
      File file = root.openNextFile();
      while (file) {
        str += file.name();
        str += " / ";
        str += file.size();
        str += "\r\n";
        file = root.openNextFile();
      }
      // Serial.print(str);
    }
    //Faça a leitura de um arquivo e retorne um vetor com todas as linhas.
    vector <String> readFile(String path) {
      vector <String> file_lines;
      String content;
      File myFile = SPIFFS.open(path.c_str(), "r");
      if (!myFile) {
        myFile.close();
        return {};
      }
      // Serial.println("###################### - FILE- ############################");
      while (myFile.available()) {
        content = myFile.readStringUntil('\n');
        file_lines.push_back(content);
        // Serial.println(content);
      }
      // Serial.println("###########################################################");
      myFile.close();
      return file_lines;
    }
    //Faça a busca de um usuario pelo ID e pela INFO.
    int findUser(vector <String> users_data, String id, String info) {
      String newID = "<td>" + id + "</td>";
      String newinfo = "<td>" + info + "</td>";
    
      for (int i = 0; i < users_data.size(); i++) {
        if (users_data[i].indexOf(newID) > 0 || users_data[i].indexOf(newinfo) > 0) {
          return i;
        }
      }
      return -1;
    }
    //Adiciona um novo usuario ao sistema
    bool addNewUser(String id, String data) {
      File myFile = SPIFFS.open(FILENAME, "a+");
      if (!myFile) {
        // Serial.println("Erro ao abrir arquivo!");
        myFile.close();
        return false;
      } else {
        // myFile.printf("<tr><td>%s</td><td>%s</td>\n", id.c_str(), data.c_str());
        // Serial.println("Arquivo gravado");
      }
      myFile.close();
      return true;
    }
    //Remove um usuario do sistema
    bool removeUser(int user_index) {
      vector <String> users_data = readFile(FILENAME);
      if (user_index == -1)//Caso usuário não exista retorne falso
        return false;
    
      File myFile = SPIFFS.open(FILENAME, "w");
      if (!myFile) {
        // Serial.println("Erro ao abrir arquivo!");
        myFile.close();
        return false;
      } else {
        for (int i = 0; i < users_data.size(); i++) {
          if (i != user_index);
            // myFile.println(users_data[i]);
        }
        // Serial.println("Usuário removido");
      }
      myFile.close();
      return true;
    }
    //Esta função substitui trechos de paginas html marcadas entre %
    String processor(const String& var) {
      String msg = "";
      if (var == "TABLE") {
        msg = "<table><tr><td>RFID Code</td><td>User Info</td><td>Delete</td></tr>";
        vector <String> lines = readFile(FILENAME);
        for (int i = 0; i < lines.size(); i++) {
          msg += lines[i];
          msg += "<td><a href=\"get?remove=" + String(i + 1) + "\"><button>Excluir</button></a></td></tr>"; //Adiciona um botão com um link para o indice do usuário na tabela
        }
        msg += "</table>";
      }
      else if (var == "SUCESS_MSG")
        msg = sucess_msg;
      else if (var == "FAILURE_MSG")
        msg = failure_msg;
      return msg;
    }
    
    void setup() {
      Serial.begin(115200);
    
      pinMode(LED_GREEN, OUTPUT);
      pinMode(LED_RED, OUTPUT);
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_RED, LOW);
    
      SPI.begin();
      mfrc522.PCD_Init();   // Inicia MFRC522
    
      // Inicialize o SPIFFS
      if (!initFS())
        return;
      listAllFiles();
      readFile(FILENAME);
    
      //  GSheet.printf("ESP Google Sheet Client v%s\n\n", ESP_GOOGLE_SHEET_CLIENT_VERSION);  // Imprime a versão da biblioteca utilizada //<<sheets
    
    
      // Conectando ao Wi-Fi
      WiFi.begin(ssid, password);
      while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Conectando WiFi..");
     // Configura o fuso horário (GMT-3:00 para São Paulo, Brasil)
    //  setTimeOffset(-3 * 3600);
         timeClient.begin();
      }
    Serial.println(WiFi.localIP());

    client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

      //sheets
     // Set the callback for Google API access token generation status (for debug only)
        GSheet.setTokenCallback(tokenStatusCallback);  // Define a função de callback para o status do token de autenticação
    
        // Set the seconds to refresh the auth token before expire (60 to 3540, default is 300 seconds)
        GSheet.setPrerefreshSeconds(10 * 60);  // Define o intervalo para atualização do token de autenticação (10 minutos)
    
        // Begin the access token generation for Google API authentication
        GSheet.begin(CLIENT_EMAIL, PROJECT_ID, PRIVATE_KEY);  // Inicia a geração do token de autenticação
      //sheets
    
      
      //Rotas.
      server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
        rfid_card = "";
        request->send(SPIFFS, "/home.html", String(), false, processor);
      });
      server.on("/home", HTTP_GET, [](AsyncWebServerRequest * request) {
        rfid_card = "";
        request->send(SPIFFS, "/home.html", String(), false, processor);
      });
      server.on("/sucess", HTTP_GET, [](AsyncWebServerRequest * request) {
        request->send(SPIFFS, "/sucess.html", String(), false, processor);
      });
      server.on("/warning", HTTP_GET, [](AsyncWebServerRequest * request) {
        request->send(SPIFFS, "/warning.html");
      });
      server.on("/failure", HTTP_GET, [](AsyncWebServerRequest * request) {
        request->send(SPIFFS, "/failure.html");
      });
      server.on("/deleteuser", HTTP_GET, [](AsyncWebServerRequest * request) {
        if (removeUser(index_user_for_removal)) {
          sucess_msg = "Usuário excluido do registro.";
          request->send(SPIFFS, "/sucess.html", String(), false, processor);
        }
        else
          request->send(SPIFFS, "/failure.html", String(), false, processor);
        return;
      });
      server.on("/stylesheet.css", HTTP_GET, [](AsyncWebServerRequest * request) {
        request->send(SPIFFS, "/stylesheet.css", "text/css");
      });
      server.on("/rfid", HTTP_GET, [](AsyncWebServerRequest * request) {
        request->send_P(200, "text/plain", rfid_card.c_str());
      });
      server.on("/get", HTTP_GET, [] (AsyncWebServerRequest * request) {
        vector <String> users_data = readFile(FILENAME);
    
        if (request->hasParam("info")) {
          info_data = request->getParam("info")->value();
          info_data.toUpperCase();
          // Serial.printf("info: %s\n", info_data.c_str());
        }
        if (request->hasParam("rfid")) {
          id_data = request->getParam("rfid")->value();
          // Serial.printf("ID: %s\n", id_data.c_str());
        }
        if (request->hasParam("remove")) {
          String user_removed = request->getParam("remove")->value();
          // Serial.printf("Remover o usuário da posição : %s\n", user_removed.c_str());
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
          // Serial.printf("Usuário numero %d ja existe no banco de dados\n", user_index);
          failure_msg = "Ja existe um usuário cadastrado.";
          request->send(SPIFFS, "/failure.html", String(), false, processor);
        }
      });
      server.on("/logo.jpg", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/logo.jpg", "image/jpg");
    });
      
      server.onNotFound(notFound);
      // Inicia o serviço
      server.begin();
    
       //Configure time //<<sheets
        configTime(0, 0, ntpServer);  // Configura o tempo utilizando o servidor NTP //<<sheets
      
    }
    
    // Declare as variáveis globais fora de qualquer função
    String uidTag;  // Variável global para armazenar o UID da tag
    String usuarioEncontrado;  // Variável global para armazenar o usuário encontrado
    
    
    void loop() {
     
      if (!client.connected()) {
    reconnect();
  }
  client.loop();
     
      // Procure por novos cartões.
      if (!mfrc522.PICC_IsNewCardPresent()) {
        return;
      }
      //Faça a leitura do ID do cartão
      if (mfrc522.PICC_ReadCardSerial()) {
        Serial.print("UID da tag :");
        String rfid_data = "";
        for (uint8_t i = 0; i < mfrc522.uid.size; i++)
        {
          // Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
          // Serial.print(mfrc522.uid.uidByte[i], HEX);
          rfid_data.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
          rfid_data.concat(String(mfrc522.uid.uidByte[i], HEX));
        }
        // Serial.println();
        rfid_data.toUpperCase();
        rfid_card = rfid_data;
    
        //Carregue o arquivo de cadastro
        vector <String> users_data = readFile(FILENAME);
        //Faça uma busca pelo id
        int user_index = findUser(users_data, rfid_data, "");
    
        uidTag = rfid_data;  // Utilize o valor da variável rfid_data como UID da tag
       // usuarioEncontrado = user_index < 0 ? "" : String(user_index);  // Utilize o valor da variável user_index como usuário encontrado, se for maior ou igual a zero; caso contrário, use uma string vazia
    //   usuarioEncontrado = users_data[user_index][1];
    //usuarioEncontrado.replace("<td>", ""); // Remove a tag inicial <td>
    //usuarioEncontrado.replace("</td>", ""); // Remove a tag final </td>
       
      client.publish(mqtt_topic, rfid_card.c_str());

        Serial.println("uidTag = " + uidTag);
        Serial.println("usuarioEncontrado = " + usuarioEncontrado);
    
      //   int user_index = findUser(users_data, id_data, info_data);
        
        if (user_index < 0) {
          // Serial.printf("Nenhum usuário encontrado\n");
          digitalWrite(LED_RED, HIGH);
        }
        else {
          // Serial.printf("Usuário %d encontrado\n", user_index);
          digitalWrite(LED_GREEN, HIGH);
    
          // Extrair a informação adicional do usuário
        int start = users_data[user_index].indexOf("<td>") + 25;  // Índice do início das informações
        int end = users_data[user_index].indexOf("</td>", start);  // Índice do final das informações
        if (start >= 0 && end >= 0) {
            info_data = users_data[user_index].substring(start, end);  // Extrair a informação entre as tags <td>
            // Serial.println("Informação adicional:");
            // Serial.println(info_data);
        } else {
            Serial.println("Erro ao extrair informação adicional do usuário");
        }
        }
        delay(1000);
        digitalWrite(LED_GREEN, LOW);
        digitalWrite(LED_RED, LOW);
        // Serial.println("SAINDO");
      }
    
      //sheets
      // Call ready() repeatedly in loop for authentication checking and processing
        bool ready = GSheet.ready();  // Verifica se a autenticação com a API do Google Sheets está pronta
    
        if (ready && millis() - lastTime > timerDelay){
            lastTime = millis();  // Atualiza o tempo do último envio
    
            FirebaseJson response;
    
            // Serial.println("\nAppend spreadsheet values...");
            // Serial.println("----------------------------");
    
            FirebaseJson valueRange;
    
            // New BME280 sensor readings
            // temp = bme.readTemperature();  // Lê a temperatura
            // hum = bme.readHumidity();  // Lê a umidade
            // pres = bme.readPressure()/100.0F;  // Lê a pressão e converte para hPa
            // Get timestamp
            epochTime = getTime();  // Obtém o tempo atual em formato epoch
    
            timeClient.update();
    
            // Obtém a hora atual em formato Unix timestamp
      time_t now = timeClient.getEpochTime();
     
    
      // Converte o timestamp para hora local
      struct tm *localTime = localtime(&now);
     
    
            // Serial.print("Hora atual: ");
            // Serial.println(timeClient.getFormattedTime());
            // Serial.println(localTime);
    
            char timeStr[20]; // Espaço para 20 caracteres, ajuste se necessário
    sprintf(timeStr, "%02d/%02d/%04d %02d:%02d:%02d",
            localTime->tm_mday, localTime->tm_mon + 1, localTime->tm_year + 1900,
            localTime->tm_hour, localTime->tm_min, localTime->tm_sec);
    
    
            valueRange.add("majorDimension", "COLUMNS");
            valueRange.set("values/[0]/[0]", epochTime);  // Define o valor de tempo na planilha
            valueRange.set("values/[1]/[0]", String(uidTag));  // Define o valor de temperatura na planilha
            valueRange.set("values/[2]/[0]", String(info_data));  // Define o valor de umidade na planilha
            valueRange.set("values/[3]/[0]", timeClient.getFormattedTime());  // Define o valor de pressão na planilha
    
    valueRange.set("values/[4]/[0]", String(timeStr));  // Define o valor de pressão na planilha
    
            // For Google Sheet API ref doc, go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets.values/append
            // Append values to the spreadsheet
            bool success = GSheet.values.append(&response /* returned response */, spreadsheetId /* spreadsheet Id to append */, "Sheet1!A1" /* range to append */, &valueRange /* data range to append */);
            if (success){
                // response.toString(Serial, true);  // Imprime a resposta do envio na serial
                valueRange.clear(); 
                Serial.println("Dados enviados com sucesso para a planilha!");
                 }
            else{
                // Serial.println(GSheet.errorReason());
                // Serial.println("Erro ao enviar dados para a planilha!");
            }
            // Serial.println();
            // Serial.println(ESP.getFreeHeap());
            
        }
    }
    
    void tokenStatusCallback(TokenInfo info){
        if (info.status == token_status_error){
            // GSheet.printf("Token info: type = %s, status = %s\n", GSheet.getTokenType(info).c_str(), GSheet.getTokenStatus(info).c_str());
            // GSheet.printf("Token error: %s\n", GSheet.getTokenError(info).c_str());
        }
        else{
            // GSheet.printf("Token info: type = %s, status = %s\n", GSheet.getTokenType(info).c_str(), GSheet.getTokenStatus(info).c_str());
        }
      //sheets
    }