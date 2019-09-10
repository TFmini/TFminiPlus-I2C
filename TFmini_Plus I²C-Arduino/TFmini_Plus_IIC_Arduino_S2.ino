/*This is a reference code about Arduino UNO receive TFmini-Plus I²C Data from I²C bus
 * Arduino is Master, TFminiPlus-I²C is slave. Master send command to TFmini Plus
 * TFminiPlus needs hardware verson equal or above 1.3.5 and firmware 1.9.0.
 * Before test IIC,you should config TFminiPlus from TTL default to IIC mode.
 * Author:zhaomingxin@Benewake
 * Update date:2019.6.26
 * This is for free.
 */
#include <Wire.h>

void setup() {
  Wire.begin();        // join i2c bus (address optional for master)
  Serial.begin(115200);  // start serial for output print
  pinMode(LED_BUILTIN, OUTPUT);//LED
  GetLidarVersionFromIIC(0x10);//Get #0x10 Lidar's firmware version.
  delay(100);
  SetLidarFrequenceFromIIC(0x10 , 100);//Set #0x10 Lidar's output frame rate to 100Hz. //If you don't need to set the frame rate, please comment this line.
  delay(100);
  SaveLidarSetFromIIC(0x10);//Save #10 Lidar`s set.
  delay(100);
  
}

void loop() {
  Get_LidarDatafromIIC(0x10);//Get #10 Lidar measurment result from IIC;
  //Get_LidarDatafromIIC(0x11);//if you connect
  //Get_LidarDatafromIIC(0x12);//if you connect
  //Get_LidarDatafromIIC(0x13);//if you connect
  
  delay(250);
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(250); 
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
}

void Get_LidarDatafromIIC(unsigned char address){
  char i = 0; 
  unsigned char rx_buf[9] = {0}; 
  unsigned char check_sum = 0;
  Wire.beginTransmission(address); // Begin a transmission to the I2C Slave device with the given address. 
  Wire.write(0x5A); // see product mannual table 11:Obtain Data Frame
  Wire.write(0x05); // 
  Wire.write(0x00); // 
  Wire.write(0x01); // 
  Wire.write(0x60); // 
  Wire.endTransmission(1);  // Send a STOP Sign
  Wire.endTransmission(0);  // Send a START Sign
  Wire.requestFrom(address, 9 , 1); // request 9 bytes from slave device address

  //Read IIC data and save to rx_buf[]
  Serial.print("Address=0x");
  Serial.print(address,HEX);
  Serial.print(":   ");
   while ( Wire.available()) 
  { 
    rx_buf[i] = Wire.read(); // received one byte 
    Serial.print("0x");
    Serial.print(rx_buf[i],HEX);
    Serial.print(";");
    i++; 
  }
  //Judge and print result via serial
    for(i=0;i<8;i++)
      check_sum += rx_buf[i];
   if(rx_buf[0] == 0x59 && rx_buf[1] == 0x59 &&  rx_buf[8] == check_sum)
   {
      Serial.print("---------->");
      Serial.print("Distance=");
      Serial.print(rx_buf[3]*256+rx_buf[2]);
      Serial.print(";");
      Serial.print("Strength=");
      Serial.print(rx_buf[5]*256+rx_buf[4]);
   }
   else
   {
      Serial.print("Maybe something wrong to Get Lidar`s data.");   
   }
  Serial.print("\r\n"); 
}

void GetLidarVersionFromIIC(unsigned char address)
{
  char i = 0; 
  unsigned char rx_buf[7] = {0}; 
  unsigned char check_sum = 0;
  Wire.beginTransmission(address); // Begin a transmission to the I2C Slave device with the given address. 
  Wire.write(0x5A); // see product mannual table 11:Obtain Data Frame
  Wire.write(0x04); // 
  Wire.write(0x01); // 
  Wire.write(0x5f); // 
  Wire.endTransmission(1);  // Send a STOP Sign 
  delay(100);
  Wire.endTransmission(0);  // Send a START Sign 
  Wire.requestFrom(address, 7 , 1); // request 9 bytes from slave device address
  //Read IIC data and save to rx_buf[]
   while ( Wire.available()) 
  { 
    rx_buf[i] = Wire.read(); // received one byte 
    Serial.print("0x");
    Serial.print(rx_buf[i],HEX);
    Serial.print(";");
    i++;
   }
   //Judge and print result via serial
   for(i=0;i<6;i++)
      check_sum += rx_buf[i];
   if(rx_buf[0] == 0x5A && rx_buf[1] == 0x07 && rx_buf[2] == 0x01 && rx_buf[6] == check_sum)
   {
      Serial.print("    The Lidar firmware version is v");
      Serial.print(rx_buf[5],HEX);
      Serial.print(".");
      Serial.print(rx_buf[4],HEX);
      Serial.print(".");
      Serial.print(rx_buf[3],HEX);    
   }
   else
   {
      Serial.print("Check version error!");
   }

    
  Serial.print("\r\n"); 
}

void SetLidarFrequenceFromIIC(unsigned char address,byte frequence)
{
  char i = 0; 
  unsigned char rx_buf[6] = {0}; 
  unsigned char check_sum = 0;
  unsigned char fre_L , fre_H;
  fre_L = frequence;
  fre_H = frequence>>8;
  Wire.beginTransmission(address); // Begin a transmission to the I2C Slave device with the given address. 
  Wire.write(0x5A); // see product mannual table 11:Obtain Data Frame
  Wire.write(0x06); // 
  Wire.write(0x03); // 
  Wire.write(fre_L); // 
  Wire.write(fre_H); // 
  Wire.write(0x5A+0x06+0x03+fre_L+fre_H); // 
  Wire.endTransmission(1);  // Send a STOP Sign 
  delay(100);
  Wire.endTransmission(0);  // Send a START Sign 
  Wire.requestFrom(address, 6 , 1); // request 9 bytes from slave device address
  //Read IIC data and save to rx_buf[]
   while ( Wire.available()) 
  { 
    rx_buf[i] = Wire.read(); // received one byte 
    //Print the rx_buf data via serial
    Serial.print("0x");
    Serial.print(rx_buf[i],HEX);
    Serial.print(";");
    i++; 
   }
   //Judge and print result via serial
   for(i=0;i<5;i++)
    check_sum += rx_buf[i];
   if(rx_buf[0] == 0x5A && rx_buf[1] == 0x06 && rx_buf[2] == 0x03 && rx_buf[5] == check_sum)
   {
      Serial.print("    Lidar's frame rate has been set to ");
      Serial.print(rx_buf[3] + rx_buf[4]*256,DEC);
      Serial.print("Hz");
   }
   else
   {
      Serial.print("Set frame rate error!");
   }

  Serial.print("\r\n"); 

}

void SaveLidarSetFromIIC(unsigned char address)
{
  char i = 0; 
  unsigned char rx_buf[5] = {0}; 
  Wire.beginTransmission(address); // Begin a transmission to the I2C Slave device with the given address. 
  Wire.write(0x5A); // see product mannual table 11:Obtain Data Frame
  Wire.write(0x04); // 
  Wire.write(0x11); // 
  Wire.write(0x6f); // 
  Wire.endTransmission(1);  // Send a STOP Sign 
  delay(100);
  Wire.endTransmission(0);  // Send a START Sign 
  Wire.requestFrom(address, 5 , 1); // request 9 bytes from slave device address
  //Read IIC data and save to rx_buf[]
   while ( Wire.available()) 
  { 
    rx_buf[i] = Wire.read(); // received one byte 
    Serial.print("0x");
    Serial.print(rx_buf[i],HEX);
    Serial.print(";");
    i++; 
   }
   //Judge and print result via serial
   if(rx_buf[0] == 0x5A && rx_buf[1] == 0x05 && rx_buf[2] == 0x11 && rx_buf[3] == 0x00 && rx_buf[4] == 0x70)
   {
       Serial.print("    Lidar's set has been saved");
   }
   else
   {
      Serial.print("Save error!");
   }

  Serial.print("\r\n"); 
}
