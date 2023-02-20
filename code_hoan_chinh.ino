#include <Arduino.h>
#include <ArduinoJson.h>
#include <string.h>
#include <stdio.h>
#include <WiFi.h> 
#include <WebSocketsServer.h> 
#include <EEPROM.h>
#include <analogWrite.h>
#include <HTTPClient.h>
#include <string.h>
#include <time.h>

#define LA_rotate 16
#define RA_rotate 17
#define EncoderA_1 2
#define EncoderA_2 4
#define LE_rotate 26
#define RE_rotate 27
#define EncoderE_1 12
#define EncoderE_2 14
int current_pos = 0;
int current_posA = 0;
int current_posE = 0;
int hope_pos = 0;
int error_pos = 0;
int space_num = 0;

String current_Motor = "";
int speed_Motor = 0;
int pos_First = 0;
int pos_Last = 1500;
int num_Turn = 0;
int turn = 0;
int total_Time = 0;
String name_User = "";
String sex_User = "";
String age_User = "";
String name_Hand = "";
String name_Bone = "";
String state = "";

String ID_SCRIP = "AKfycbzqLK48Hl5-ToQnpiCc0fjTyPw2eEtNGDIt3fQdwsF1GweBkXFm9ovcVIdMk9ADhnnaDQ"; //ID của google sheets
String Url;

#define LENGTH(x) (strlen(x) + 1)
#define EEPROM_SIZE 200
#define WiFi_rst 0
String ssidd;
String psss;

unsigned long time_Start;
unsigned long rst_millis;
unsigned long spd_millis;

const char *ssid =  "Hello ESP32";   //Tên wifi mà ESP32 phát  
const char *pass =  "12345678"; //Mật khẩu wifi mà ESP32 phát

const char* ntpServer = "pool.ntp.org"; //Server lấy thời gian online
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 3600;

char timeNow[50];
char timeHour[3];
char timeMin[3];
char timeSec[3];
String timenow;
int timehournow;
int timeminnow;
int timesecnow;
int timehourold;
int timeminold;
int timesecold;

bool start_flag = true;

WebSocketsServer webSocket = WebSocketsServer(81); //Khai báo server dùng websocket ở port 81

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) { //Các sự kiện của websocket
    String cmd = "";
    
    switch(type) {
        case WStype_DISCONNECTED:{ //Khi ngắt kết nối với Client
            Serial.println("Websocket is disconnected");
            break;}
        case WStype_CONNECTED:{{ //Khi kết nối với Client
            Serial.println("Websocket is connected");
            Serial.println(webSocket.remoteIP(num).toString()); 
            webSocket.sendTXT(num, "connected");}
            break;}
        case WStype_TEXT:{ //Khi nhận được dữ liệu
            cmd = "";
            for(int i = 0; i < length; i++) {
                cmd = cmd + (char) payload[i]; 
            } //Vì websocket nhận từng kí tự nên phải cộng từng kí tự vào chuỗi cmd
            Serial.println(cmd);
            
            const size_t capacity = JSON_OBJECT_SIZE(2) +256; //Khai báo size của chuỗi JSON
            DynamicJsonDocument JSON(capacity); //Khai báo biến JSON
            DeserializationError error = deserializeJson(JSON,cmd); //Khai báo kiểm tra lỗi JSON
            if (error)
              {return;}
            else 
            {     
            serializeJsonPretty(JSON,cmd); //Chuyển đổi kiểu JSON
            Serial.println();
              String Data_State = JSON["STATE"]; //Lấy giá trị của trường "STATE" tương tự các trường bên dưới cũng như vậy
              if (Data_State != "null") {
                Serial.println(Data_State); state = Data_State;
                if (state == "play"){
                  if (start_flag) { //start_flag = true tức là bắt đầu, start_flag = false tức là đã bắt đầu, có cờ này để thời gian không bị đếm lại mỗi khi lỡ gửi nhầm STATE bằng start
                    time_Start = millis();
                    printLocalTime();
                    start_flag = false;
                  }
                }
              }
              String Data_Name = JSON["NAME"];
              if (Data_Name != "null"){
                Serial.println(Data_Name); name_User = Data_Name;
                name_User.replace(" ","%20");
              }
              String Data_Age = JSON["AGE"];
              if (Data_Age != "null"){
                Serial.println(Data_Age); age_User = Data_Age;
              }
              String Data_Sex = JSON["SEX"];
              if (Data_Sex != "null"){
                Serial.println(Data_Sex); sex_User = Data_Sex;
              }
              String Data_Hand = JSON["HAND"];
              if (Data_Hand != "null"){
                Serial.println(Data_Hand); name_Hand = Data_Hand; 
                if (Data_Hand == "Left") {
                  current_Motor = "up";
                }
                else {
                  current_Motor = "down";
                }
                Serial.println(current_Motor);
              }
              String Data_Bone = JSON["BONE"];
              if (Data_Bone != "null"){
                Serial.println(Data_Bone); name_Bone = Data_Bone; 
              }
              String Data_Speed = JSON["SPEED"];
              if (Data_Speed != "null"){
                Serial.println(Data_Speed); speed_Motor = Data_Speed.toInt();
              }
              String Data_First = JSON["POS FIRST"];
              if (Data_First != "null"){
                Serial.println(Data_First); 
                if (Data_First == "Set"){
                  name_Bone == "Arm"?pos_First = current_posA:pos_First = current_posE;
                }
              }
              String Data_Last = JSON["POS LAST"];
              if (Data_Last != "null"){
                Serial.println(Data_Last); 
                if (Data_Last == "Set"){
                  name_Bone == "Arm"?pos_Last = current_posA:pos_Last = current_posE;
                }
              }
              String Data_Num = JSON["NUM TURN"];
              if (Data_Num != "null"){
                Serial.println(Data_Num); num_Turn = Data_Num.toInt();
              }
             }
             webSocket.sendTXT(num, cmd+":success");
             //send response to mobile, if command is "poweron" then response will be "poweron:success"
             //this response can be used to track down the success of command in mobile app.
            break;}
        case WStype_FRAGMENT_TEXT_START:{
            break;}
        case WStype_FRAGMENT_BIN_START:{
            break;}
        case WStype_BIN:
            break;
        default:
            break;
    }
}

void setup() {
  pinMode(WiFi_rst, INPUT);
  pinMode(EncoderA_1, INPUT);
  pinMode(EncoderA_2, INPUT);
  pinMode(LA_rotate,OUTPUT);
  pinMode(RA_rotate,OUTPUT);
  pinMode(EncoderE_1, INPUT);
  pinMode(EncoderE_2, INPUT);
  pinMode(LE_rotate,OUTPUT);
  pinMode(RE_rotate,OUTPUT);
   WiFi.mode(WIFI_AP_STA); //Chọn chế độ Access Point và Station
   Serial.begin(115200); //serial start
   EEPROM_Setup(); //Set up bộ nhớ EEPROM để lấy tên và mật khẩu của wifi mà ESP32 kết nối
   Wifi_Setup(); //Set up kết nối wifi cho ESP32.
   Time_Setup(); //Set up lấy thời gian online
   Serial.println("Connecting to wifi");
   delay(1000);
   IPAddress apIP(192, 168, 0, 1);   //Đặt địa chỉ IP cho ESP32 ở chế độ Access Point
   WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0)); //Đặt Subnet mask cho địa chỉ IP trên
   WiFi.softAP(ssid, pass); //Mở wifi

   webSocket.begin(); //Bắt đầu server websocket
   webSocket.onEvent(webSocketEvent); //Các sự kiện của websocket được thiết lập
   Serial.println("Websocket is started");
   spd_millis = millis();
   attachInterrupt(digitalPinToInterrupt(EncoderA_1), encoderA, RISING);
   attachInterrupt(digitalPinToInterrupt(EncoderE_1), encoderE, RISING);
   
}

void loop() {
  Serial.println(current_posA);
  Serial.println(current_posE);
  if (name_Bone == "Arm"){
    error_pos = hope_pos - current_posA;}
  else if (name_Bone == "Elbow") {
    error_pos = hope_pos - current_posE;}
   webSocket.loop(); //Vòng lặp để websocket luôn hoạt động
   if (state == "play"){
    if (millis() - spd_millis >= 1000){
      if (current_Motor == "up") {
        hope_pos += speed_Motor;
        if (hope_pos >= pos_Last){
          current_Motor = "down";
          if (name_Hand == "Right"){
            turn +=1;
            if (turn == num_Turn){
              state = "stop";
              turn = 0;
            }
          }
        }
      }
      else if (current_Motor == "down"){
        hope_pos -= speed_Motor;
        if (hope_pos <= pos_First){
          current_Motor = "up";
          if (name_Hand == "Left"){
            turn +=1;
            if (turn == num_Turn){
              state = "stop";
              turn = 0;
            }
          }
        }
      }
      spd_millis = millis();
      
      Serial.print("PID: "); Serial.println (pid_controller(error_pos,1,3,2));
      Serial.println (timenow);
    }
    Rotation(name_Bone, pid_controller(error_pos,1,3,2));
   }
   else if (state == "pause"){
    analogWrite(RA_rotate,0);
    analogWrite(LA_rotate,0);
    analogWrite(RE_rotate,0);
    analogWrite(LE_rotate,0);
   }
   else if (state == "stop"){
    analogWrite(RA_rotate,0);
    analogWrite(LA_rotate,0);
    analogWrite(RE_rotate,0);
    analogWrite(LE_rotate,0);
    webSocket.broadcastTXT("stopped");
    unsigned long total_time = millis() - time_Start;
    total_Time = (total_time/60000);
    Send_Data_To_GoogleSheet();
    start_flag = true;
    state = "";
    pos_First = 0;
    pos_Last = 1500;
    Serial.print("State: ");Serial.println(state);
   }
   Restart_Wifi();
}

void EEPROM_Setup() {
  if (!EEPROM.begin(EEPROM_SIZE)) { //Khởi tạo bộ nhớ EEPROM
    Serial.println("failed to init EEPROM");
    delay(1000);
  }
  else
  {
    ssidd = readStringFromFlash(0); // Đọc giá trị tên wifi mà ESP32 từng kết nối ở địa chỉ 0
    Serial.print("SSID = ");
    Serial.println(ssidd);
    psss = readStringFromFlash(40); // Đọc giá trị mật khẩu wifi mà ESP32 từng kết nối ở địa chỉ 40
    Serial.print("psss = ");
    Serial.println(psss);
  }
}

void Wifi_Setup() {
  //
  WiFi.begin(ssidd.c_str(), psss.c_str()); //Kết nối wifi với tên và mật khẩu đã đọc ở EEPROM

  delay(3500);   // Chờ cho đến khi ESP32 kết nối với mạng wifi

  if (WiFi.status() != WL_CONNECTED) // Nếu wifi không được kết nối
  {
    //Bắt đầu chế độ Smart Config, ESP32 ở chế độ AP để điện thoại kết nối với ESP32
    WiFi.mode(WIFI_AP_STA);
    WiFi.beginSmartConfig();

    Serial.println("Waiting for SmartConfig.");
    while (!WiFi.smartConfigDone()) {
      delay(500);
      Serial.print(".");
    }
    //Sau khi kết nối thành công điện thoại sẽ gửi tên, mật khẩu wifi qua 
    Serial.println("");
    Serial.println("SmartConfig received.");

    //Chờ cho đến khi ESP32 kết nối với wifi vừa nhận
    Serial.println("Waiting for WiFi");
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }

    Serial.println("WiFi Connected.");

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    //Ghi tên và mật khẩu wifi vừa kết nối vào bộ nhớ Flash
    ssidd = WiFi.SSID();
    psss = WiFi.psk();
    Serial.print("SSID:");
    Serial.println(ssidd);
    Serial.print("PSS:");
    Serial.println(psss);
    Serial.println("Store SSID & PSS in Flash");
    writeStringToFlash(ssidd.c_str(), 0); //Tên wifi ghi ở địa chỉ 0
    writeStringToFlash(psss.c_str(), 40); //Mật khẩu wifi ghi ở địa chỉ 40
  }
  else
  {
    Serial.println("WiFi Connected");
  }
}

void Time_Setup(){ //Phương thức set up thời gian
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); //Thiết lập server
  printLocalTime(); //Lấy thời gian hiện tại
  timehourold = timehournow;
  timeminold = timeminnow;
  timesecold = timesecnow;
  delay(1000);
}

void Rotation (String bone,int value){
  if (value > 0){
    if (bone == "Arm"){
      analogWrite(RA_rotate,0);
      analogWrite(LA_rotate,value);}
    else if (bone == "Elbow"){
      analogWrite(RE_rotate,0);
      analogWrite(LE_rotate,value);
    }
  }
  else if (value <0){
    if (bone == "Arm"){
      analogWrite(RA_rotate,value);
      analogWrite(LA_rotate,0);}
    else if (bone == "Elbow"){
      analogWrite(RE_rotate,value);
      analogWrite(LE_rotate,0);
    }
  }
}

int pid_controller (int error, int kp, int ki, int kd){
  int d_error,i_error;
  static int prev_error;
  long int temp;
  d_error = error-prev_error;
  i_error += error;
  if (i_error >= 200){
    i_error = 200;
  }
  else if (i_error <=-200){
    i_error = -200;
  }
  prev_error = error;
  temp = kp*error + ki*i_error + kd*d_error;
  if (temp >= 255){
    temp = 255;
  }
  else if (temp <= -255){
    temp = -255;
  }
  return temp;
}

void encoderA(){
  if(digitalRead(EncoderA_2) == HIGH){
    current_posA++;
  }else{
    current_posA--;
  }
}

void encoderE(){
  if(digitalRead(EncoderE_2) == HIGH){
    current_posE++;
  }else{
    current_posE--;
  }
}

void Send_Data_To_GoogleSheet(){ //Phương thức gửi data lên google sheet
  Url = "https://script.google.com/macros/s/" + ID_SCRIP + "/exec?date=" + timenow + "&name=" + name_User + "&age=" + String(age_User) + "&sex=" + sex_User + "&handname=" + name_Hand + "&bonename=" + name_Bone + "&time=" + String(total_Time);
  httpGETRequest(Url.c_str());
}


String httpGETRequest(const char* Url) //Hàm hỗ trợ gửi data lên google sheet
{
  HTTPClient http; 
  http.begin(Url); //Khởi tạo giao thực http
  int responseCode = http.GET(); //Phương thức là get phản hồi của google sheet
  String responseBody = "{}";
  if (responseCode > 0) //Nếu phản hồi lớn hơn 0 tức là gửi data lên google sheet thành công
  {
    Serial.print("responseCode:");
    Serial.println(responseCode);
    responseBody = http.getString();
  }
  else //Nếu phản hồi bé hơn 0 tức là gửi data lên google sheet thất bại
  {
    Serial.print("Error Code: ");
    Serial.println(responseCode);
  }
  http.end();
  return responseBody;
}

void printLocalTime() {  //Phương thực lấy thời gian thực
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) { //Nếu lấy thời gian lỗi
    Serial.println("Failed to obtain time");
    return;
  } //Nếu lấy thời gian thành công
  strftime(timeNow, 50, "%d/%m/%Y", &timeinfo); //Chuyển đổi định dạng thời gian
  strftime(timeHour, 3, "%H", &timeinfo);
  timehournow = String(timeHour).toInt() + 7;
  if (timehournow >= 24){
    timehournow = timehournow - 24;
  }
  strftime(timeMin, 3, "%M", &timeinfo);
  timeminnow = String(timeMin).toInt();
  strftime(timeSec, 3, "%S", &timeinfo);
  timesecnow = String(timeSec).toInt();
  timenow = String(timehournow) + ":" + String(timeminnow) + "%20" + String(timeNow);
  
}

void writeStringToFlash(const char* toStore, int startAddr) { //Phương thức ghi một chuỗi kí tự và bộ nhớ Flash
  int i = 0;
  for (; i < LENGTH(toStore); i++) { //Nó sẽ ghi từng kí tự một của chuỗi
    EEPROM.write(startAddr + i, toStore[i]); 
  }
  EEPROM.write(startAddr + i, '\0');
  EEPROM.commit();
}


String readStringFromFlash(int startAddr) { //Phương thức đọc một chuỗi kí tự và bộ nhớ Flash
  char in[128]; 
  int i = 0;
  for (; i < 128; i++) { //Nó sẽ đọc từng kí tự một của chuỗi rồi ghép vào biến in
    in[i] = EEPROM.read(startAddr + i);
  }
  return String(in);
}

void Restart_Wifi() {
rst_millis = millis();
  while (digitalRead(WiFi_rst) == LOW)
  {
    
  }
  // Nếu nút Boot được nhấn hơn 3s 
  if (millis() - rst_millis >= 3000)
  {
    Serial.println("Reseting the WiFi credentials");
    writeStringToFlash("", 0); // Ghi giá trị trống vào địa chỉ 0
    writeStringToFlash("", 40); // Ghi giá trị trống vào địa chỉ 40
    Serial.println("Wifi credentials erased");
    Serial.println("Restarting the ESP");
    delay(500);
    ESP.restart();            //Hàm restart ESP32
  }
}
