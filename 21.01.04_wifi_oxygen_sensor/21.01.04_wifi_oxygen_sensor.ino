// Interfacing Arduino with LCD example
 //Rhys M. Adams
 //
 
// include the library code
#include <LiquidCrystal.h>
#include <WiFiNINA.h>
#include <time.h>

//wifi client stuff
char ssid[] = "guest";        // your network SSID (name)
char pass[] = "";    // your network password (use for WPA, or use as key for WEP)

int keyIndex = 0;                 // your network key Index number (needed only for WEP)
int status = WL_IDLE_STATUS;      //connection status

const int array_len = 100; //number of time points to store
float times[array_len]; //times of measurements
float ppms[array_len]; // measurements
int count = 0; // next measurement index
float old_millis = 0; // previous measurement time
float equilibrium_time = 10000; //time to equilibrate voltages. The O2 sensor uses feedback loops and the voltages take time to reach steady state,
//so it can improve measurements if you let the voltages equilibrate.
float time_step = 60000 + equilibrium_time; // time between measurements
int measurement_cycle = 0; //toggles between 0 and 1 after a time step has passed

WiFiServer server(80);            //server socket
IPAddress ip;
WiFiClient client = server.available();

/////////////////////
// LCD is connected to Arduino as: LiquidCrystal lcd(RS, E, D4, D5, D6, D7)
LiquidCrystal lcd(11, 12, 6, 7, 8, 9);

//pwm pins for setting opamp voltage baselines
//circuit diagram can be found at 
//https://www.sgxsensortech.com/content/uploads/2014/08/A1A-EC_SENSORS_AN2-Design-of-Electronics-for-EC-Sensors-V4.pdf

const int ic2_voltage = 3 ; //relative ground voltage pin, approximately 2.9 V
int vground = 151;//average PWM is 2.9 V

const int ic1_voltage = 2 ; //voltage pin, approximately 3.5 V
int v_set_voltage = vground+31; //31 = 600 mv difference
    
//sensor pins for opamp outputs
const int vout = A1 ;//measuremnt pin

////////////////////

float r_gain = 10000;
float scaling_ppm = 4 * 1e-7 * r_gain;


void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  ip = WiFi.localIP();
  lcd.setCursor(0,0);
  lcd.print(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");

  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);
}

void enable_WiFi() {
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
  }

  String fv = WiFi.firmwareVersion();
  if (fv < "1.0.0") {
    Serial.println("Please upgrade the firmware");
  }
}


void connect_WiFi() {
  // attempt to connect to Wifi network:
  int StatusWiFi=WiFi.status();
  Serial.println(StatusWiFi);
  if(StatusWiFi!=3||(WiFi.localIP()[0]==0))  //if the connection gets dropped, try to reconnect
  {
      WiFi.disconnect();
      lcd.setCursor(0,0);
      lcd.print("looking for wifi ");
      delay(1000);
      //WiFiServer server(80);
      Serial.print("Attempting to connect to SSID: ");
      Serial.println(ssid);
      status = WiFi.begin(ssid, pass);
      
      Serial.println(status);
  }
  Serial.println("connected");

}

void printWEB() {

  if (client) {                             // if you get a client,
    Serial.println("new client");           // print a message out the serial port
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            //create table
            client.print("<html><body>");
            client.print("<table class=\"dataTable text-light dataTables-sidebar overflow-x-auto\" cellspacing=\"30\" cellpadding=\"0\"><thead></thead><tbody>");
            
            client.print("<tr><td>time(s)</td><td>oxygen(ppm)</td></tr>");
            for(int ii=0;ii<array_len;ii++){
              int curr_index = (ii+count)%array_len;
                if(times[curr_index] != 0){
                  client.print("<tr><td>");
                  client.print(times[curr_index]);
                  client.print("</td><td>");
                  client.print(ppms[curr_index]);
                  client.print("</td></tr>");
              }
            
            }
            client.print("</table></tbody></body></html>");
            
            
            // break out of the while loop:
            break;
          }
      }
    }
    // close the connection:
    client.stop();
  }
}

float get_oxygen_voltage(){
    //Take the average voltage of the oxygen sensor
    int num_iterations = 10;
    float counter_val = 0;
    float out_val = 0;
    
    for(int ii=0;ii<num_iterations;ii++){
      out_val += float(analogRead(vout)) * 5. / 1023.;
      //counter_val += float(analogRead(vcount));// * 5. / 1023.;
      delay(200);
      }
    out_val /= num_iterations;  
    ////////////////////////////////////////
    //Let the oxygen sensor rest
    ///////////////////////////////////////
    analogWrite(ic1_voltage,0) ;
    analogWrite(ic2_voltage,0) ;  
    return out_val;
}

void setup() {
  // Set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Set the cursor to column 6, line 1
  lcd.setCursor(2, 0);
  // Print a message to the LCD.
  lcd.print("O2!");
  
  lcd.setCursor(2, 1);
  lcd.print("Yes please!");
  
  pinMode(ic1_voltage,OUTPUT) ;
  pinMode(ic2_voltage,OUTPUT) ;  

  Serial.begin(9600);
  WiFi.disconnect();
  enable_WiFi();
  connect_WiFi();
  server.begin();
  printWifiStatus();
}

void loop() {   
    if(millis() - old_millis > time_step){//if enough time has passed since the last measurement cycle, start a new measurement
      old_millis = millis(); //reset the old_millis time stamp
      analogWrite(ic1_voltage,v_set_voltage); //turn on the O2 sensor
      analogWrite(ic2_voltage,vground);
      measurement_cycle = 1; //measurement cycle has started
    }
    if(measurement_cycle == 1)
    {
      if(millis() - old_millis > equilibrium_time)//if the voltage has equilibrated take measurements
      {
        float delta_v = get_oxygen_voltage();//get the measured voltages
        analogWrite(ic1_voltage,0);//turn O2 sensor off
        analogWrite(ic2_voltage,0);
        measurement_cycle = 0;//change state to not measuring
        ////////////////////////////////////////
        //output voltage
        ///////////////////////////////////////
        lcd.clear();//update lcd
        lcd.setCursor(0,0);
        lcd.print(ip);
        lcd.setCursor(0,1);
        lcd.print("~");
        lcd.setCursor(1,1);
        lcd.print(delta_v/scaling_ppm);
        lcd.setCursor(12, 1);
        lcd.print("ppm");
        
        // record data
        times[count%array_len] = millis()/1000.;
        ppms[count%array_len] = delta_v/scaling_ppm;
        count += 1;
        count %= array_len;
        
      }
    }

        
    connect_WiFi();
    printWifiStatus();

    client = server.available();//check if someone is querying the arduino "website"
    if (client) {
      lcd.setCursor(0,0);
      lcd.print("connecting    ");
      printWEB(); //if somone is connecting, send them a table of measurements
      delay(5000);
      lcd.setCursor(0,0);
      lcd.print(ip);
    }
  delay(1000);  
}
