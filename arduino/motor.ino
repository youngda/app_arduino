// Defining variables
const int stepPin = 7; 
const int dirPin = 6;
String state = ""; 
int currentHeading=0;
int currentAngle=0;
int lastAngle=0;
int angle=0;
int rotate=0;
int runContinuously=0;
String mode = "Manual";
boolean dirRotation = HIGH;
int rotSpeed = 1500;
 
void setup() {
  // Sets the two pins as Outputs
  pinMode(stepPin,OUTPUT); 
  pinMode(dirPin,OUTPUT);
  Serial.begin(38400); // Default communication rate of the Bluetooth module
}
void loop() {
  delayMicroseconds(1);
  if(Serial.available() > 0){ // Checks whether data is comming from the serial port
    state = Serial.readString(); // Reads the data from the serial port
 }
 // When Auto Button is pressed
 if (mode == "Auto") {
  if (state == "Reverse") {
    delay(10);
    if (dirRotation == HIGH) {
      dirRotation = LOW;
    }
    else {
      dirRotation = HIGH;
    }  
    digitalWrite(dirPin,dirRotation);
    delay(10);
    state = "";
  }
  rotSpeed = state.toInt();
  if (rotSpeed >= 300 && rotSpeed <= 3000) {
  digitalWrite(stepPin,HIGH); 
  delayMicroseconds(rotSpeed); 
  digitalWrite(stepPin,LOW); 
  delayMicroseconds(rotSpeed);
  }
  else {
  digitalWrite(stepPin,HIGH); 
  delayMicroseconds(1500); 
  digitalWrite(stepPin,LOW); 
  delayMicroseconds(1500);
  }
  
  if (state == "Manual"){
    mode = state;
  }
 }
 // When Program is in Manual mode
 else if (mode == "Manual"){ 
 currentHeading = state.toInt();
 //Serial.println(angle);
 //Serial.println(state);
 if (currentHeading < 0 ){
  currentHeading = 360+currentHeading;
 }
 currentAngle = map(currentHeading,0,359,0,200);
 digitalWrite(dirPin,HIGH); // Enables the motor to move in a particular direction
  // Makes 200 pulses for making one full cycle rotation
  if (currentAngle != lastAngle){
    if(currentAngle > lastAngle){  
      rotate = currentAngle - lastAngle;  
      for(int x = 0; x < rotate; x++) {
      digitalWrite(stepPin,HIGH); 
      delayMicroseconds(500); 
      digitalWrite(stepPin,LOW); 
      delayMicroseconds(500); 
      }
    }
    if(currentAngle < lastAngle){  
      rotate = lastAngle - currentAngle; 
      digitalWrite(dirPin,LOW); //Changes the rotations direction
      for(int x = 0; x < rotate; x++) {
      digitalWrite(stepPin,HIGH); 
      delayMicroseconds(500); 
      digitalWrite(stepPin,LOW); 
      delayMicroseconds(500); 
      }
    }
  }
  lastAngle = currentAngle;
  if (state == "Auto"){
    mode = state;
  }
 }
}
