#include "FS.h"

/*********
  File parameters
*********/

const char* loghtml = "/log.html";
const char* lresponse = "HTTP/1.1 200 OK\nContent-Type: text/html; charset=utf-8\nConnection: close\n\n";
const char* lheader = "<!doctype html><html><head><title>Arrosage</title><meta http-equiv=\"refresh\" content=\"5\"></head>\n\n";

String item2write = "";

/*********
  File init
*********/

boolean initLog() {
  File f = SPIFFS.open(loghtml, "w");
  if (!f) {
    Serial.println("file open failed");
    return false;
  }
  f.print(lresponse);
  f.print(lheader);
  f.println("<body>");
  f.close();
}

/*********
  Write log
*********/

boolean writeLogln(String text2w, NTPClient timeClient) {
  timeClient.forceUpdate();
  Serial.println(text2w);
  item2write = item2write + text2w;
  File f = SPIFFS.open(loghtml, "a+");
  if (!f) {
    Serial.println("file open failed");
    return false;
  }
  f.print(timeClient.getFormattedTime());
  f.print("&nbsp;");
  f.print(item2write);
  f.print("<br/>");
  f.close();
  item2write = "";
}
boolean writeLog(String text2w) {
  Serial.print(text2w);  
  item2write = item2write + text2w;
}

void webOutputLog(WiFiClient client) {
  File f = SPIFFS.open(loghtml, "r");
  if (!f) {
    Serial.println("file open failed");
    // return false;
  }
  while (f.available()) {
     client.write(f.read());
  }
  f.close();
  client.write("</body></html>");
  // return true;
}
