#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

//Receiver Mac Address
//24:6F:28:0B:7C:98
uint8_t broadcastAddress[] = {0x24, 0x6F, 0x28, 0x0B, 0x7C, 0x98};



// Struct for data necessary to send
typedef struct {
  int id = 2;
  int value = 22;
} dataToSend;

// Initialise object with struct
dataToSend dataPacket;

//Sensor interval (Must match interval in hub code) (unit: seconds)
unsigned long sensorInterval = 30;

//Marker if code is still waiting for activation
bool activated = false;


//New mac address (must match hub code)
uint8_t newMAC[] = {0x32, 0xAE, 0xA4, 0x07, 0x0D, 0x66};

esp_now_send_status_t sendStatus = ESP_NOW_SEND_SUCCESS;

//_________________________________________________________

void sendData(){
  // Send message via ESP-NOW
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &dataPacket, sizeof(dataToSend));

  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
}


// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nStatus of last packet sent:\t"); // \r moves cursor to start, \n is new line, \t is tabulator
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery success" : "Delivery fail");

  sendStatus = status;

}

void changeMac(){


  Serial.print("[OLD] ESP32 Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
  
  esp_wifi_set_mac(ESP_IF_WIFI_STA, &newMAC[0]);
  
  Serial.print("[NEW] ESP32 Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
  
}

void wifiInitialize(){
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  changeMac();

  // Initialise ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);

  // Register peer
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  
}

//Activated function when data is received
void OnDataReceived(const uint8_t * mac, const uint8_t *incomingData, int len) {
  activated = true;
  Serial.println("Data received!");
  
  
}

void setup() {
  // Initialise Serial Monitor
  Serial.begin(115200);
  wifiInitialize();

  //Register function for received data
  esp_now_register_recv_cb(OnDataReceived);

  while (! activated){
    delay(10);
  }

}



void loop() {
  //Make sure hub is ready to receive
  delay(500 * dataPacket.id + 500);

  for(int i = 0; i < 5; i++){
    sendData();
  }

  /*
  while (sendStatus != ESP_NOW_SEND_SUCCESS){
  sendData();
  }
  */

  Serial.println("I sleep");

  //Deep sleep esp for power saving
  unsigned long timeMultiplier = 1000000;
  esp_sleep_enable_timer_wakeup(sensorInterval * timeMultiplier);
  esp_deep_sleep_start();
  
}
