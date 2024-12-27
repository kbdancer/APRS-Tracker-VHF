#include <SoftwareSerial.h>
#include <TinyGPSPlus.h>
#include <LibAPRS_Tracker.h>

static const uint32_t GPSBaud = 9600;
static const int RX_PIN = 12, TX_PIN = 11;
SoftwareSerial ss(RX_PIN, TX_PIN);
TinyGPSPlus gps;

#define PTT_PIN 8

// APRS settings
char comment[] = "APRS Tracker for VHF radio, https://github.com/kbdancer/APRS-Tracker-VHF";
char callsign[] = "BG5UXC";
int SSID = 9;

char last_lat[9] = "0000.00N";
char last_lon[10] = "00000.00E";

char lat[9] = "0000.00N";
char lon[10] = "00000.00E";

int ptt_begin_delay = 1000;
int ptt_end_delay = 800;

int gpsCourse = 90;
int gpsSpeed = 0;

bool debug = false;
bool sending = false;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PTT_PIN, OUTPUT);
  APRS_init();
  APRS_setCallsign(callsign, SSID);
  APRS_setDestination((char *)"AP5UXC", 0);
  APRS_setPath1((char *)"WIDE1", 1);
  APRS_setPath2((char *)"WIDE2", 1);
  APRS_setPreamble(350);
  APRS_setTail(150);
  APRS_setSymbol('<');  // do not use "", must be ''
  // Serial.begin(115200);
  if(!debug){ss.begin(GPSBaud);}
}

void loop() {
  digitalWrite(PTT_PIN, 1);
  if(debug){
    debug = false;
    sendPosition();
  }else{
    while (ss.available() > 0) {
      if (!sending) {
        gps.encode(ss.read());
        if (gps.location.isUpdated() && gps.location.isValid()) {
          updatePosition();
        }
      }
    }
  }
}

void updatePosition() {
  int temp = 0;
  // Convert and set latitude NMEA string Degree Minute Hundreths of minutes ddmm.hh[S,N].
  double d_lat = gps.location.lat();
  double dm_lat = 0.0;

  if (d_lat < 0.0) {
    temp = -(int)d_lat;
    dm_lat = temp * 100.0 - (d_lat + temp) * 60.0;
  } else {
    temp = (int)d_lat;
    dm_lat = temp * 100 + (d_lat - temp) * 60.0;
  }

  dtostrf(dm_lat, 7, 2, lat);

  if (dm_lat < 1000) { lat[0] = '0'; }
  if (dm_lat < 100) { lat[1] = '0'; }
  if (dm_lat < 10) { lat[2] = '0'; }

  if (d_lat >= 0.0) {
    lat[7] = 'N';
  } else {
    lat[7] = 'S';
  }

  // Convert and set longitude NMEA string Degree Minute Hundreths of minutes ddmm.hh[E,W].
  double d_lon = gps.location.lng();
  double dm_lon = 0.0;

  if (d_lon < 0.0) {
    temp = -(int)d_lon;
    dm_lon = temp * 100.0 - (d_lon + temp) * 60.0;
  } else {
    temp = (int)d_lon;
    dm_lon = temp * 100 + (d_lon - temp) * 60.0;
  }

  dtostrf(dm_lon, 8, 2, lon);

  if (dm_lon < 10000) { lon[0] = '0'; }
  if (dm_lon < 1000) { lon[1] = '0'; }
  if (dm_lon < 100) { lon[2] = '0'; }
  if (dm_lon < 10) { lon[3] = '0'; }
  if (d_lon >= 0.0) {
    lon[8] = 'E';
  } else {
    lon[8] = 'W';
  }
  if(strcmp(last_lat, lat) == 0 && strcmp(last_lon, lon) == 0){
    Serial.println("Position not update.");
  }else{
    sending = true;
    ss.flush();
    ss.end();
    
    memcpy(last_lat, lat, strlen(lat) + 1);
    memcpy(last_lon, lon, strlen(lon) + 1);
    sendPosition();
  }    
}

void sendPosition() {
  gpsCourse = (int)gps.course.deg();
  gpsSpeed = (int)gps.speed.knots();
  if (gpsSpeed > 999) { gpsSpeed = 999; }

  APRS_setCourse(gpsCourse);
  APRS_setSpeed(gpsSpeed);

  APRS_setLat(lat);
  APRS_setLon(lon);
  
  digitalWrite(PTT_PIN, 0);
  digitalWrite(LED_BUILTIN, 1);
  delay(ptt_begin_delay);
  APRS_sendLoc(comment, strlen(comment), 'c');
  delay(ptt_end_delay);
  digitalWrite(PTT_PIN, 1);
  digitalWrite(LED_BUILTIN, 0);
  delay(10000);
  ss.begin(GPSBaud);
  sending = false;
}
