
#include "SPI.h"        // Lib for SPI interface
#include "max6675.h"    // Lib for thermal sensor
#include "Ucglib.h"     // Lib for 240x360 display

int xPosTemp = 21;      // X Position of Temperature for Graph. See comments.
int OldTempHeight;      // Old Height of Temperature for Graph. This is to know from which point we need to draw next section of the graph
float temp;             // Current head temperature

float starttime = 0;    // Variable, which stores time passed from the beginning of brewing. Perhaps woth changing to long long type.

int firstrun = 1;       // Variable to check if brewing function was run first time. It is used to clear chart from any previous results.
int buttonState = 0;    // Variable to see, if coffeemaker motor is still running.

MAX6675 thermocouple(4, 3, 8);  // Initialization of MAX6675. Gonna be replaced with something more exact. Like DS18B20.

Ucglib_ILI9341_18x240x320_HWSPI ucg(6,7,5);  // Initialization of Display controller and pins. STRONGLY recommended to use Hardware SPI if available. On Arduino UNO it sits on 13 and 11 pins.

void setup() {
  Serial.begin(9600); // For debugging

  pinMode(2, INPUT); // Input to detect if brewing has started.
  
  ucg.begin(UCG_FONT_MODE_SOLID); // All symbols will have black background.

  ucg.clearScreen();  // Clear screen
  ucg.setRotate90();  // We need landscape orientation

  // Real color array is BGR, not RGB. Perhaps, a controller issue.

  // Pressure Gauge pic
  ucg.setColor(100, 100, 100);
  ucg.drawCircle(19, 16, 11, UCG_DRAW_ALL);
  ucg.drawDisc(19, 16, 1, UCG_DRAW_ALL);
  ucg.drawLine(19, 16, 24, 14);
  ucg.drawBox(18, 27, 3, 4);
  ucg.drawBox(13, 30, 13, 2);

  // Thermometer pic
  ucg.drawRFrame(120,5, 7, 26, 2);
  ucg.drawCircle(123, 27, 6, UCG_DRAW_ALL);
  ucg.setColor(0, 0, 200);
  ucg.drawBox(121, 12, 5, 15);
  ucg.drawDisc(123, 27, 5, UCG_DRAW_ALL);
  ucg.setColor(100, 100, 100);
  ucg.drawHLine(124, 9, 2);
  ucg.drawHLine(124, 12, 2);
  ucg.drawHLine(124, 15, 2);
  ucg.drawHLine(124, 18, 2);

  // Sand Clock Pic
  ucg.drawHLine(199, 4, 14);
  ucg.drawHLine(199, 5, 14);
  ucg.drawTriangle(199,7, 212,7,   205,18);
  ucg.drawTriangle(199,29, 212,29,   205,18);
  ucg.drawHLine(199, 30, 14);
  ucg.drawHLine(199, 31, 14);

  // Draw Graph base
  ucg.setFont(ucg_font_helvR08_tf);
  ucg.drawVLine(20, 40, 199);
  ucg.drawHLine(20, 239, 272);
  ucg.drawVLine(291, 40, 199);

  ucg.setPrintPos(2,44);
  ucg.print("12");
  ucg.drawHLine(18, 40, 2);

  ucg.setPrintPos(7,94);
  ucg.print("9");
  ucg.drawHLine(18, 90, 2);

  ucg.setPrintPos(7,144);
  ucg.print("6");
  ucg.drawHLine(18, 140, 2);

  ucg.setPrintPos(7,194);
  ucg.print("3");
  ucg.drawHLine(18, 190, 2);

  ucg.drawHLine(292, 40, 2);
  ucg.drawHLine(292, 80, 2);
  ucg.drawHLine(292, 120, 2);

  ucg.setPrintPos(298,45);
  ucg.print("100");

  ucg.setPrintPos(299,85);
  ucg.print("90");

  ucg.setPrintPos(299,125);
  ucg.print("80");

  ucg.setFont(ucg_font_helvR14_hf);
}


void loop() {
  
  // The main script basically prints sensors data and constantly checks if coffeemaker pump is running or not.
  
  sensordata();  // Call a function, which prints pressure and temperature data in stand-by mode.

  firstrun = 1;  // 1 is set for the next IF script to run and clear chart before plotting
  xPosTemp = 21; // X axis value for the first point of the chart
  
  buttonState = digitalRead(2);  // Let's check if the coffeemaker pump is running. This is a workaround to evade usage of interrupts.
  starttime = (millis());        // Let's remember the exact time we checked the pump
  
  while(buttonState == HIGH){    // A-HA! We detected, that coffeemaker pump is running, meaning that we'd better start plotting sensors data on a chart
     if (firstrun > 0) {         // Let's check if it is first time, when pump was activated.
        firstrun = 0;            // Variable is set to 0, so on next run this script is gonna be skipped.
        
        // For cleaning chart are from previous results, we basically draw a black rectangle over it. Same goes over timer data.
       
        ucg.setColor(0, 0, 0);
        ucg.drawBox(21, 40, 270, 199);
        ucg.drawBox(217, 10, 45, 18);
        ucg.setColor(255, 255, 255);
       
        OldTempHeight = map(thermocouple.readCelsius()*100, 0, 10000, 0, 400); // This is to ensure the first segment of the chart will not be drawn from Y=0
     }

    brewtime(); // Call a chart drawing function
    buttonState = digitalRead(2); // Chek if pump is still running.

  }
}


void brewtime() {

  // This function draws chart and prints sensors data.
  
  // Print Timer Data 
  ucg.setPrintPos(217, 25);
  ucg.print((millis() - starttime)/1000);
  
  sensordata(); // We still need to print actual sensors data, why not to export it to separate function and use when neccessary?
  
  // Print Temperature chart
  
  int tempHeight = map(temp*100, 0, 10000, 0, 400);  // Temperature is from 0 to 100, but virtual chart size for it is 400 pixels in heigth. Here we use map function to adapt data. We mulitply incomming data by 100 as MAP can not process float point numbers.

  ucg.setColor(0, 0, 255);
  ucg.drawLine(xPosTemp, 440-OldTempHeight,(xPosTemp+4), 440-tempHeight);  // The line is drawn from previous temp probe to current. Don't forger that virtual chart size is bigger than real screen, so thats why we have "440-" part.
  ucg.setColor(200, 200, 200);

  xPosTemp = xPosTemp + 4;     // Next point will be 4 pixels right. On my hardware it makes total chart scale equal to nearly 30 seconds, ideal for espresso brewing.
  OldTempHeight = tempHeight;  // Store last Y temperature position for the next cycle.
  
  
  // Print Pressure chart. To be done later.

}


void sensordata() {

  temp = thermocouple.readCelsius();

  //Print Pressure Gauge data
  ucg.setPrintPos(36, 25);
  ucg.print("1.3 | 9.2");

  // Print Temperature Data 
  ucg.setPrintPos(136, 25);
  ucg.print(temp);
  
  // Print Pressure data. To be done later.
  
}
