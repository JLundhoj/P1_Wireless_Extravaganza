//Include libraries
  #include <U8g2lib.h>
  #include <esp_now.h>
  #include <WiFi.h>
  #include <SPI.h>

//Initialize screen software and define pins
  U8G2_SH1106_128X64_NONAME_F_4W_HW_SPI   u8g2  (U8G2_R0,     5,           22,           17);


//Variable declarations
//______________________________________________

//Container for received data
  //Struct type
  typedef struct dataContainer {
    int id = 0;
    int value = 0;
  } dataContainer;

  //Actual container
  dataContainer receivedData;

  //Container for incoming data
  dataContainer incomingData;

//Sensor amount
  const int sensorAmount = 2;

//Sensor interval (Must match interval in sensor code) (unit: seconds)
  const int sensorInterval = 30;

//Array for all sensor data
  int Data[sensorAmount + 1];

//Array for string
  char printString[] = "Sensor ";

//Variable to indicate data collection and start shutdown
  bool activated = false;

//Sensor mac address
  //{0x32, 0xAE, 0xA4, 0x07, 0x0D, 0x66}
  uint8_t sensorMac[] = {0x32, 0xAE, 0xA4, 0x07, 0x0D, 0x66};


//Function declarations
//___________________________________________________________

/* printData(Pointer, Size)
 * Prints all data of an array if said data is not = -1. 
 * Pointer: The pointer of the array to be printed
 * Size: Amount of datapoints in the array
 */
void printData(int *arrPointer, int Size){
  for(int i = 0; i < Size; i++){
    if (arrPointer[i] != -1){
      Serial.print("Sensor ");
      Serial.print(i);
      Serial.print(" reads: ");
      Serial.println(arrPointer[i]);    
    }
  }
}


/* arrReset(Pointer, Size)
 * Sets all data of an array to be = -1. 
 * Pointer: The pointer of the array to be printed
 * Size: Amount of datapoints in the array
 */
void arrReset(int *arrPointer, int Size){
    for(int i = 0; i < Size; i++){
      arrPointer[i] = -1;
  }
}

/*bootMessage()
 * Displays:"
 * Welcome
 * No data yet
 * " on oled screen
 */
void bootMessage(){
  u8g2.clearBuffer();
  u8g2.drawStr(5, 10, "Welcome");
  u8g2.drawStr(5, 25, "No data yet");
  u8g2.sendBuffer();
}

/* showData(Pointer, Size)
 * Shows every sensor and it's data unless the data is -1
 * Pointer: Pointer to array containing received data
 * Size: Size of array/maximum amount of sensors
 */
void showData(int *arrPointer, int Size){
  //Clear screenbuffer
  u8g2.clearBuffer();

  for(int i = 0; i < Size; i++){
    if (arrPointer[i] != -1){

      //Reset string to contain beginning of text
      String Temp = "Sensor ";

      //Construct rest of string to be printed on screen
      Temp.concat(i);
      Temp.concat(": ");
      Temp.concat(arrPointer[i]);

      //Construct char-array from string since function to print on screen requires char-array
      Temp.toCharArray(printString, 15);

      //Draw string on display
      u8g2.drawStr(5, 10 * i, printString);
    }
  }
  //Send screenbuffer. This actually shows the constructed content on the screen
  u8g2.sendBuffer();
}

//Activated function when data is received
void OnDataReceived(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&receivedData, incomingData, sizeof(receivedData));

  //Saves received data in array place corresponding to sensor id
  Data[receivedData.id] = receivedData.value;

  //Prints out all received data
  //Serial.println();
  
  //printData(Data, sensorAmount + 1);
  showData(Data, sensorAmount + 1);

  activated = true;
  
}

/* Activated function after sending data
 * Prints "Delivery success" if data sent was received and "Delivery fail" if not
 */
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nStatus of last packet sent:\t"); // \r moves cursor to start, \n is new line, \t is tabulator
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery success" : "Delivery fail");
}

/* wifiInitialize()
 * Initializes wifi and esp-now 
 * Must be run once at the start of setup()
 */
void wifiInitialize(){  
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

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
  memcpy(peerInfo.peer_addr, sensorMac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  
}

void sendData(){
  //Make dummy data to send
  dataContainer dataToSend;
  
  // Send message via ESP-NOW
    esp_err_t result = esp_now_send(sensorMac, (uint8_t *) &dataToSend, sizeof(dataToSend));

  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
}

//Funtion code begins
//________________________________________________

void setup() {
  Serial.begin(115200);

  wifiInitialize();

  //Register function for received data
  esp_now_register_recv_cb(OnDataReceived);

  //Initialize screen
  u8g2.begin();
  //Choose font
  u8g2.setFont(u8g2_font_6x13_tf);


  //Reset data array to only contain -1
  arrReset(Data, sensorAmount + 1);

  

  bootMessage();

  
  
}

void loop() {

  WiFi.mode(WIFI_STA);

  arrReset(Data, sensorAmount + 1);

  delay(100);

  //Let sensors know to transmit data
  sendData();

  //Wait until data is received
  while (! activated){
    delay(10);
  }

  activated = false;
  
  //Make sure all data is received
  delay(3000);

  WiFi.mode(WIFI_OFF);

  Serial.println("WIFI off");
  Serial.println(sensorInterval);  
  delay((sensorInterval) * 1000);

  Serial.println("Going again");

}
