#include <Arduino.h>
#include <LoRa.h>
#include <T2WhisperNode.h>
#include <RH_RF69.h>

T2Message myMsg;
uint8_t radioBuf(T2_MESSAGE_HEADERS_LEN + T2_MESSAGE_MAX_DATA_LEN);
uint8_t mySerialNumber = 6;
uint8_t idNode;
uint8_t idNetwork;
uint16_t idChannel;

void initialisation() {
  LoRa.setPins(10,7,2);

  if (!LoRa.begin(868E6)) {
    Serial.println("Starting LoRa failed !");
    while(1);
  }
}

void receptionDonnees(uint8_t message[]) {
  int packetSize = LoRa.parsePacket();
  uint8_t i = 0;

  if (packetSize) {
    while (LoRa.available()) {
      message[i++] = (char) LoRa.read();
    }
  }
}

void emissionDonnees(uint8_t message[]) {
  LoRa.beginPacket();
  LoRa.write(message, 10);
  LoRa.endPacket();
}

int sendLORA(int idx, int src, int dst, int sdx, int cmd, const char *data, int len) {
  uint8_t radioBufLen = 0;

  myMsg.idx = idx;
  myMsg.src = src;
  myMsg.dst = dst;
  myMsg.sdx = sdx;
  myMsg.cmd = cmd;
  myMsg.len = len;

  memcpy(myMsg.data, data, len);

  myMsg.getSerializedMessage(&radioBuf, &radioBufLen);

  // for debug purpose
  myMsg.printMessage();

  LoRa.beginPacket();
  LoRa.write(&radioBuf, radioBufLen);
  LoRa.endPacket();
  delay(10);
  return 1;
}

int sendGiveMeANodeID() {
  char buf[10];
  int len;

  itoa(mySerialNumber, buf, 10);
  len = strlen(buf);
  buf[len] = ';';
  len++;

  return sendLORA(0x00, 0x00, 0x01,0x01, 0x00, buf, len);
}

int sendGiveMeAChannelAndField() {
  char buf[10];
  return sendLORA(idNetwork, idNode, 0x01,0x02, 0x00, buf, 0x00);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  initialisation();
  sendGiveMeANodeID();
  
  int size = 250;
  uint8_t receivedMess[size];
  bool flagReceived = false;
  uint8_t idWhisperNode;

  // Récupération de id node et id network
  while (!flagReceived) {
    receptionDonnees(receivedMess);
    myMsg.setSerializedMessage((uint8_t *) receivedMess, size);

    if (myMsg.src == 0x01 && myMsg.sdx == 0x01 && myMsg.cmd == 0x01 && myMsg.len >= 6) {
      // parsing message 
      idNode = atoi(strtok((char*)myMsg.data, ";"));
      idNetwork = atoi(strtok(NULL, ";"));
      idWhisperNode = atoi(strtok(NULL, ";"));

      if (idWhisperNode == mySerialNumber) {
        flagReceived = true;
        Serial.println(idNode);
        Serial.println(idNetwork);
      }
    }
  }
  
  Serial.println("Bonjour, je suis la reponse à la demande d'id\n");
  myMsg.printMessage();

  // Demande du channel
  sendGiveMeAChannelAndField();

  // Récupération de id node et id network
  uint8_t receivedMessForChannel[size];
  flagReceived = false;

  while (!flagReceived) {
    receptionDonnees(receivedMessForChannel);
    myMsg.setSerializedMessage((uint8_t *) receivedMessForChannel, size);

    if (myMsg.src == 0x01 && myMsg.sdx == 0x02 && myMsg.cmd == 0x01 && myMsg.dst == idNode && myMsg.idx == idNetwork) {
      // parsing message 
      idChannel = atoi(strtok((char*)myMsg.data, ";"));
      Serial.println(idChannel);
      Serial.println(atoi(strtok(NULL, ";")));

      flagReceived = true;
    }
  }  
  Serial.println("Bonjour, je suis la reponse à la demande de channel.\n");
  myMsg.printMessage();
}

void loop() {
  // put your main code here, to run repeatedly:
  // Serial.println("please show me something");
}