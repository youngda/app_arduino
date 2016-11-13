
/*
Start up a very mini web server
 ******* This requires the Mega 2560 board *****
 ******* This requires the Mega 2560 board *****
 ******* This requires the Mega 2560 board *****
 ******* This requires the Mega 2560 board *****
 ******* This requires the Mega 2560 board *****
 ******* This requires the Mega 2560 board *****
 ******* This requires the Mega 2560 board *****
 */

// This section for QD_TFT180X SPI LCD Modle
//http://www.QDtech.net
//http://qdtech.taobao.com
//Param1:Value Can be:QD_TFT180A/QD_TFT180B/QD_TFT180C
//Param2 instructions:Connect to LCD_Pin SDA/SDI/MOSI(it means LCD_Model Pin_SDA/SDI/MOSI Connect to Arduino_UNO Pin11)
//Param3 instructions:Connect to LCD_Pin SCL/CLK/SCLK(it means LCD_Model Pin_SCL/CLK/SCLK Connect to Arduino_UNO Pin10)
//Param4 instructions:Connect to LCD_Pin CS/CE(it means LCD_Model Pin_CS/CE Connect to Arduino_UNO Pin9)
//Param5 instructions:Connect to LCD_Pin RST/RESET(it means LCD_Model Pin_RST/RESET Connect to Arduino_UNO Pin12)
//Param6 instructions:Connect to LCD_Pin RS/DC(it means LCD_Model Pin_RS/DC Connect to Arduino_UNO Pin8)
#include <UTFT.h>

UTFT myGLCD(QD_TFT180C,11,10,9,12,8);   // Remember to change the model parameter to suit your display module!
extern uint8_t SmallFont[];             // Declare which fonts we will be using
//extern uint8_t BigFont[];             // Declare which fonts we will be using
//extern uint8_t SevenSegNumFont[];     // Declare which fonts we will be using
#define LCD_NETWORK_ROW  101
#define LCD_IPADDR_ROW   114
#define LCD_GRAY_ROW     101
#define LCD_SERVED_ROW   14
#define LCD_CHANGES_ROW  28
#define LCD_RESETS_ROW   42
#define LCD_APP_REQ_ROW  56

#define TIMEOUT      5000 // mS
#define GREENLED     4
#define REDLED       5
#define BLUELED      3
#define ESP_RESET    22
#define BAUDRATE     19200

//*** Literals and vars used in the message queue ***
#define QUE_SIZE     8
#define HTML_REQUEST      1
#define FAVICON_REQUEST   2
#define PUT_REQUEST       3
#define QUE_INTERVAL    300
int QueIn=0;
int QueOut=0;
int CommandQue[QUE_SIZE];
int CommandQueIPD_CH[QUE_SIZE];
//String CommandQue[QUE_SIZE];
//String WaitQue[QUE_SIZE];
float LastCommandSent=0;
float LastQueEntered=0;
String CIPSendString="";
String HTTPHeader;
String HTMLCode1;  
String HTMLCode2;
String PutResponse ="HTTP/1.0 200 OK \r\n"
                    "Date: Fri, 31 Dec 1999 23:59:59 GMT\r\n"
                    "Content-type: text/html\r\n"
                    "Content-length: 0\r\n"
                    "Connection: Closed\r\n"
                    "\r\n";
//HTTP/1.1 201 Created\r\n"

//*** States of the LEDs ***
boolean RED_State = false;
boolean GREEN_State = false;
boolean BLUE_State = false;


//*** Used for server stat section ***
float NumberOfResets=0;
float NumberServed=0;
float NumberIconReqs=0;
float NumberLEDRequest=0;
float NumberBusyS=0;
float NumberAppReq=0;

void setup()  
{
  pinMode(REDLED,OUTPUT); 
  pinMode(GREENLED,OUTPUT);
  pinMode(BLUELED,OUTPUT);
  pinMode(ESP_RESET,OUTPUT);
  
  digitalWrite(ESP_RESET,HIGH);
  Serial.begin(115200);         // Manual interface port
  Serial1.begin(BAUDRATE);        // Port to ESP8266 Wifi Module

  //*** Set up the background and text template ***//
  SetUpLCDBaseDisplay();
  
  //*** Send AT commands to bring up ESP8266 as server ***
  InitWifiModule();
  
  Serial1.println("AT+CWJAP?");   // Send first time manually to update LCD sooner
  delay(50);
  Serial1.println("AT+CIFSR");   // Send first time manually to update LCD sooner
}

//***** Sets up the base LCD Display *****
void SetUpLCDBaseDisplay(){
  myGLCD.InitLCD();
  myGLCD.setFont(SmallFont);
  myGLCD.clrScr();

  //*** Create blue title bar with name ***
  myGLCD.setColor(0, 0, 255);          // Blue for interior of rectangle coming up
  myGLCD.fillRect(0, 0, 159, 13);      // Draw a solid rectangle from upper left = (0,0) to bottom right (159,13) 
  myGLCD.setBackColor(0, 0, 255);      // Blue background for upcoming text
  myGLCD.setColor(255, 255, 255);      // White lettering for upcoming text
  myGLCD.print("Pete's ESP8266 Hack", RIGHT, 1);    // Write the text right justified at row 1
  
  //*** Create gray bar at bottom with status ***
  myGLCD.setColor(64, 64, 64);                 // Grey 
  myGLCD.fillRect(0, LCD_GRAY_ROW, 159, 127);  // Gray box at bottom of screen
  myGLCD.setBackColor(64, 64, 64);             // Gray background for upcoming text
  myGLCD.setColor(255, 255, 255);              // White lettering for upcoming text
  myGLCD.print("Network:", LEFT, LCD_NETWORK_ROW);  
  myGLCD.print("...init",RIGHT, LCD_NETWORK_ROW);
  myGLCD.print("IP Add:", LEFT, LCD_IPADDR_ROW);
  myGLCD.print("...init",RIGHT, LCD_IPADDR_ROW);

  //** Create labels for key server stats  
  myGLCD.setBackColor(0, 66, 0);          // Darkish Green  ... note: lettering is set to white from last above still
  myGLCD.print("LED Changes: " + String(int(NumberLEDRequest)), RIGHT, LCD_CHANGES_ROW);
  myGLCD.print("Web Served: " + String(int(NumberServed)), RIGHT, LCD_SERVED_ROW);
  myGLCD.print("ESP8266 RSTS: " + String(int(NumberOfResets)), RIGHT, LCD_RESETS_ROW);  
  myGLCD.print("App Reqs: " + String(int(NumberAppReq)), RIGHT, LCD_APP_REQ_ROW);  
  
}

void loop(){
  String IncomingString="";
  char SingleChar;
  boolean StringReady = false;


  //*** Handle each character that is coming in from the ESP8266 ****
  while(Serial1.available()) 
  {
    IncomingChar (Serial1.read ());
  }  
  
  //*** Allow manual commands to be given during run time. ***
  while(Serial.available())
  {
    Serial1.write(Serial.read());
  }
  
  //*** Send more queued commands if in pointer != out pointer
  if (QueIn != QueOut){
    if ((millis()-LastCommandSent > QUE_INTERVAL) && (millis()-LastQueEntered > QUE_INTERVAL)){
      ProcessQueCommand(QueOut);
      if(QueOut!=QueIn){
        QueOut++;
      }
      LastCommandSent = millis();
      if (QueOut==QUE_SIZE){
        QueOut=0;
        Serial.println("Resetting QueOut");
      }
    }else{
      //Serial.println("waiting to send");
    }
  }
  
  //*** Update the LCD screen with latest server stats if no characters to process 
  //UpdateLCDStats();
  
  //*** Setting the LED states to what are in the state vars for the case of a ESP8266 reset event ***
  SetLEDStates(); 
  
  
}


//***** Update the LCD screen with key server stats *****
void UpdateLCDStats(){
  myGLCD.setBackColor(0, 66, 0);      // Darkish green background
  myGLCD.setColor(255, 255, 255);     // White lettering
  myGLCD.print(String(int(NumberLEDRequest)), RIGHT, LCD_CHANGES_ROW);
  myGLCD.print(String(int(NumberServed)), RIGHT, LCD_SERVED_ROW);
  myGLCD.print(String(int(NumberOfResets)), RIGHT, LCD_RESETS_ROW);
  myGLCD.print(String(int(NumberAppReq)), RIGHT, LCD_APP_REQ_ROW);
}

boolean WaitForKey(String keyword){
  byte current_char   = 0;
  byte keyword_length = keyword.length();
  long deadline = millis() + TIMEOUT;

  while(millis() < deadline)
  {
    if (Serial1.available())
    {
      char ch = Serial1.read();
      IncomingChar (ch);                                    // Keep processing incoming commands
      if (ch == keyword[current_char])
        if (++current_char == keyword_length)
        {
          Serial.println("Fouund keyword: " + keyword);
         
          //***  ESP8266 seems to choke if things are sent too soon ***
          while(millis()-LastCommandSent < QUE_INTERVAL){
              if(Serial1.available()) 
              {
                IncomingChar (Serial1.read ());
              }  
          }
          return true;
        }
    }
  }
  Serial.println("Timed out on keyword: " + keyword);
  return false;
}
  


boolean SendCIPChunk(String StringToSend,int IPD_CH){
  
  Serial1.println("AT+CIPSEND=" + String(IPD_CH) + "," + String(StringToSend.length()+2));
  LastCommandSent = millis();
  WaitForKey(">");                    // Don't care if timed out for now.  Will just continue for now.
  
  if (QueOut==QueIn) return false;    // This only can happen if ESP8266 reset while waiting to send string 
 
  Serial1.println(StringToSend);
  LastCommandSent = millis();
  WaitForKey("SEND OK");
  if (QueOut==QueIn) return false;    // This only can happen if ESP8266 reset while waiting to send string 

  return true;
}


  

//***** Formulate command string and send *****
void ProcessQueCommand(int QueOut){

  switch (CommandQue[QueOut]){
    case HTML_REQUEST:
      BuildHTMLPage();
      if(!SendCIPChunk(HTTPHeader,CommandQueIPD_CH[QueOut])){   // Send the CIPSEND command for the HTTPHeader string 
        break;                                                  // returned false because there was a ESP8266 reset
      }    

      if(!SendCIPChunk(HTMLCode1,CommandQueIPD_CH[QueOut])){     // Send the CIPSEND command for HTMLCode1 
        break;                                                  // returned false because there was a ESP8266 reset
      }    

      if(!SendCIPChunk(HTMLCode2,CommandQueIPD_CH[QueOut])){     // Send the CIPSEND command for HTMLCode2 
        break;                                                  // returned false because there was a ESP8266 reset
      }    
      break;
      
    case FAVICON_REQUEST:
      //*** Send the CIPSEND command for Close ***       
      Serial1.println("AT+CIPCLOSE=" + String(CommandQueIPD_CH[QueOut]));
      WaitForKey("OK");
      break;
  
    case PUT_REQUEST:
      SendCIPChunk(PutResponse,CommandQueIPD_CH[QueOut]);     // Send the CIPSEND command to respond to Put request 
      break;

    default:
      //Nothing here yet  
      Serial.println("Should never see this");
  }
}  

//*** Builds the HTTP header + HTML code to send out ***
void BuildHTMLPage(){
  String RED_StateHTML;
  String GREEN_StateHTML;
  String BLUE_StateHTML; 
  

  //*** Pre-build HTML code to check the correct state of LED *** 
  if (RED_State){
    RED_StateHTML = "<input type=\"radio\" name=\"RedLEDState\" value=\"RED_ON\" checked=\"checked\"> ON"
                    "<input type=\"radio\" name=\"RedLEDState\" value=\"RED_OFF\"> OFF<br>";
  }else{
    RED_StateHTML = "<input type=\"radio\" name=\"RedLEDState\" value=\"RED_ON\"> ON"
                    "<input type=\"radio\" name=\"RedLEDState\" value=\"RED_OFF\" checked=\"checked\"> OFF<br>";
  }
  
  if (GREEN_State){
    GREEN_StateHTML = "<input type=\"radio\" name=\"GreenLEDState\" value=\"GREEN_ON\" checked=\"checked\"> ON"
                      "<input type=\"radio\" name=\"GreenLEDState\" value=\"GREEN_OFF\"> OFF<br>";
  }else{
    GREEN_StateHTML = "<input type=\"radio\" name=\"GreenLEDState\" value=\"GREEN_ON\"> ON"
                      "<input type=\"radio\" name=\"GreenLEDState\" value=\"GREEN_OFF\" checked=\"checked\"> OFF<br>";
  }
  
  if (BLUE_State){
    BLUE_StateHTML = "<input type=\"radio\" name=\"BlueLEDState\" value=\"BLUE_ON\" checked=\"checked\"> ON"
                     "<input type=\"radio\" name=\"BlueLEDState\" value=\"BLUE_OFF\"> OFF<br>";
  }else{
    BLUE_StateHTML = "<input type=\"radio\" name=\"BlueLEDState\" value=\"BLUE_ON\"> ON"
                     "<input type=\"radio\" name=\"BlueLEDState\" value=\"BLUE_OFF\" checked=\"checked\"> OFF<br>";
  }    
    
  //Note: Need this first since HTTP Header needs length of content
  HTMLCode1  =    "<HTML>"
                  "<HEAD><TITLE>Pete's Mini8266 Server</TITLE>"
                  "<BODY><H1>Welcome to Pete's ESP8266 \"hacking\" project</H1>"
                     "<form action=\"\" method=\"post\">"
                     "<fieldset>"
                        "<legend>Red LED State</legend>" +
                          RED_StateHTML + 
                     "</fieldset>"
                     "<fieldset>"
                        "<legend>Green LED State</legend>" +
                          GREEN_StateHTML + 
                     "</fieldset>";
  HTMLCode2  =       "<fieldset>"
                        "<legend>Blue LED State</legend>" + 
                          BLUE_StateHTML +
                     "</fieldset>"                         
                     "<fieldset>"
                        "<legend>Actions</legend>"
                        "<input type=\"submit\" name=\"LEDFormAction\" value=\"Set LED States\"> &nbsp &nbsp &nbsp <input type=\"submit\" name=\"LEDFormAction\" value=\"Get LED States\">"
                     "</fieldset>"
                     "</form>"
                  "<BR><BR>"
                  "<HR>"
                  "<H2>Server Stats</H2>"  
                  "<B>Number Servered: </B>" + String(NumberServed) +
                  "<BR><B>Number LED Changes: </B>" + String(NumberLEDRequest) +
                  "<BR><B>Number Icon Reqs: </B>" + String(NumberIconReqs) +
                  "<BR><B>Number of Resets: </B>" + String(NumberOfResets) +
                  "<BR><B>Number of Busy S: </B>" + String(NumberBusyS) +
                  "<BR><B> Up time: </B>" + String(millis()) + 
                  "</BODY></HTML>";
  //*** Build HTTP Header ***
  HTTPHeader = "HTTP/1.0 200 OK \r\n"
                  "Date: Fri, 31 Dec 1999 23:59:59 GMT \r\n"
                  "Content-Type: text/html\r\n"
                  "Content-Length: " + String(HTMLCode1.length()+HTMLCode2.length()) + "\r\n";
}



//***** Group up characters until end of line ****
void IncomingChar (const byte InChar)
{
  static char InLine [500];    //Hope we don't get more than that in one line
  static unsigned int Position = 0;

  switch (InChar)
  {
  case '\r':   // Don't care about carriage return so throw away.
    break;
    
  case '\n':   
    InLine [Position] = 0;  
    ProcessCommand(String(InLine));
    Position = 0;  
    break;

  default:
      InLine [Position++] = InChar;
  }  
} 


void ProcessCommand(String InLine){
  Serial.println("InLine: " + InLine);

  if (InLine.startsWith("+IPD,")){
    CommandQueIPD_CH[QueIn]=InLine.charAt(5)-48;    // The value returned is ASCII code.  48 is ASCII code for 0
    Serial.println("******** IPD found: " + String(CommandQueIPD_CH[QueIn]));
  }
  if (InLine.indexOf("LEDFormAction=Set+LED+States") != -1){
    ParseLEDControl(InLine);
    SetLEDStates();
    NumberLEDRequest++;
    UpdateLCDStats();
  }
  if (InLine.indexOf("LEDFormAction=Get+LED+States") != -1){
    //Do nothing sinc the "POST / " command already is sending HTML page with latest LED states
  }
  if (InLine.indexOf("GET / ") != -1) {
    CommandQue[QueIn++]=HTML_REQUEST;
    NumberServed++;
    UpdateLCDStats();
  }
  if (InLine.indexOf("POST / ") != -1) {
    CommandQue[QueIn++]=HTML_REQUEST;
    NumberServed++;
    UpdateLCDStats();
  }
  if (InLine.indexOf("favicon.ico") != -1) {  
    CommandQue[QueIn++]=FAVICON_REQUEST;
    NumberIconReqs++;
  }
  if (InLine.indexOf("System Ready") != -1) {
    Serial.println("The ESP8266 Reset for some reason");
    digitalWrite(ESP_RESET,LOW);
    delay(500);
    digitalWrite(ESP_RESET,HIGH);    
    QueOut=QueIn;                      //Clear out the command que
    InitWifiModule();
    NumberOfResets++;
    UpdateLCDStats();
  }
  if (InLine.indexOf("busy s...") != -1) {
    Serial.println("dead with busy s...   HW Reset");
    QueOut=QueIn;                      //Clear out the command que
    digitalWrite(ESP_RESET,LOW);
    delay(500);
    digitalWrite(ESP_RESET,HIGH);    
    NumberBusyS++;
    // Note: Parser should see the reset and start the InitWifiModule routine
  }
  if (InLine.indexOf("link is not") != -1) {
    Serial.println("AHAHAHAHAHAAHAHAHAHAHAH***********************************");
    QueOut=QueIn;                      // Clear out the command que so no more sending CIPSEND
  }  
  if (InLine.startsWith("192.")){
    Serial.println("Found IP address: " + InLine);
    myGLCD.setBackColor(64, 64, 64);
    myGLCD.setColor(255, 255, 255);
    myGLCD.print(InLine, RIGHT, LCD_IPADDR_ROW);
  }    
  if (InLine.indexOf("+CWJAP:") != -1) {
    InLine.replace("+CWJAP:","");      // Strip line to only network name
    InLine.replace("\"","");           // Strip line to only network name
    Serial.println("Found network: " + InLine);
    myGLCD.setBackColor(64, 64, 64);
    myGLCD.setColor(255, 255, 255);
    myGLCD.print(InLine, RIGHT, LCD_NETWORK_ROW);
  }
  
  if (QueIn==QUE_SIZE){
    Serial.println("Resetting QueIn");
    QueIn=0;
  }
  
  if (InLine.indexOf("PtcApp ") != -1) {
    CommandQue[QueIn++]=PUT_REQUEST;
    ParseCustomAppRequest(InLine);
    UpdateLCDStats();
    NumberAppReq++;
    UpdateLCDStats();
  }
  LastQueEntered = millis();
  
}

//*** This parses out the LED control strings from custom Android App and sets the appropriate state var *** 
void ParseCustomAppRequest(String InLine){
  if (InLine.indexOf("R:0")!=-1) RED_State=false;
  if (InLine.indexOf("G:0")!=-1) GREEN_State=false;
  if (InLine.indexOf("B:0")!=-1) BLUE_State=false;

  if (InLine.indexOf("R:1")!=-1) RED_State=true;
  if (InLine.indexOf("G:1")!=-1) GREEN_State=true;
  if (InLine.indexOf("B:1")!=-1) BLUE_State=true;
}

//*** This parses out the LED control strings and sets the appropriate state var *** 
void ParseLEDControl(String InLine){
  if (InLine.indexOf("RED_OFF")!=-1) RED_State=false;
  if (InLine.indexOf("GREEN_OFF")!=-1) GREEN_State=false;
  if (InLine.indexOf("BLUE_OFF")!=-1) BLUE_State=false;

  if (InLine.indexOf("RED_ON")!=-1) RED_State=true;
  if (InLine.indexOf("GREEN_ON")!=-1) GREEN_State=true;
  if (InLine.indexOf("BLUE_ON")!=-1) BLUE_State=true;
}

//*** This will set the LED control to the current state stored in the state vars ***
void SetLEDStates(){
  if (RED_State) digitalWrite(REDLED,HIGH);
  else digitalWrite(REDLED,LOW);

  if (GREEN_State) digitalWrite(GREENLED,HIGH);
  else digitalWrite(GREENLED,LOW);
  
  if (BLUE_State) digitalWrite(BLUELED,HIGH);
  else digitalWrite(BLUELED,LOW);
}  
  
  
//***** This initializes the Wifi Module as a server  *****
void InitWifiModule(){
  int CommandStep = 1;
  BlinkLED(REDLED,CommandStep,50);  
  SendCommand("AT+RST", "Ready", true);
  BlinkLED(GREENLED,CommandStep,50);
  CommandStep++;

  BlinkLED(REDLED,CommandStep,50); 
  SendCommand("AT+GMR", "OK", true);
  BlinkLED(GREENLED,CommandStep,50);
  CommandStep++;

  delay(3000);

  BlinkLED(REDLED,CommandStep,50); 
  SendCommand("AT+CIFSR", "OK", true);
  BlinkLED(GREENLED,CommandStep,50);
  CommandStep++;


  BlinkLED(REDLED,CommandStep,50); 
  SendCommand("AT+CIPMUX=1","OK",true);
  BlinkLED(GREENLED,CommandStep,50);
  CommandStep++;

  BlinkLED(REDLED,CommandStep,50); 
  SendCommand("AT+CIPSERVER=1,80","OK",true);
  BlinkLED(GREENLED,CommandStep,50);

  digitalWrite(GREENLED,HIGH);
  //----------------------------------------------------
}

void BlinkLED(int LEDPin, int NumberOfBlinks, int OnDuration)
{
  for (int x=1; x <= NumberOfBlinks ; x ++){
  digitalWrite(LEDPin,HIGH);
  delay(OnDuration);
  digitalWrite(LEDPin,LOW);
  delay(OnDuration);   
  }
}

//************** This section specific to simple AT command with no need to parse the response ************
//  Warning: Will drop any characters coming in while waiting for the indicated response.
boolean SendCommand(String cmd, String ack, boolean halt_on_fail)
{
  Serial1.println(cmd); // Send command to module

  // Otherwise wait for ack.
  if (!echoFind(ack)) // timed out waiting for ack string
    if (halt_on_fail)
      errorHalt(cmd+" failed");// Critical failure halt.
    else
      return false; // Let the caller handle it.
  return true; // ack blank or ack found
}

// Read characters from WiFi module and echo to serial until keyword occurs or timeout.
boolean echoFind(String keyword)
{
  byte current_char   = 0;
  byte keyword_length = keyword.length();

  // Fail if the target string has not been sent by deadline.
  long deadline = millis() + TIMEOUT;
  while(millis() < deadline)
  {
    if (Serial1.available())
    {
      char ch = Serial1.read();
      Serial.write(ch);
      if (ch == keyword[current_char])
        if (++current_char == keyword_length)
        {
          Serial.println();
          return true;
        }
    }
  }
  return false;  // Timed out
}

// Print error message and loop stop.
void errorHalt(String msg)
{
  Serial.println(msg);
  Serial.println("HALT");
  digitalWrite(REDLED,HIGH);
  digitalWrite(GREENLED,HIGH);
  while(true){
  };
}
