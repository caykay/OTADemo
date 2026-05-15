// Attempt to setup a basic WiFi LAN OTA - uses the BasicOTA example from https://github.com/caykay/arduino-esp32/tree/master/libraries/ArduinoOTA/examples/BasicOTA

#include <WebServer.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <LittleFS.h>

#include "views.h"

static WebServer server(80);
constexpr char* Hostname = "esp32";
constexpr char* Ssid = "****";
constexpr char* Password = "****";

static File uploadFile; // file to be uploaded to LittleFS
static bool uploadCompleted = false;

void startMDNS()
{
  if (!MDNS.begin(Hostname)) {
    Serial.println("[mDns] Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  Serial.printf("[mDns] mDNS responder started, check: %s.local\n", Hostname);
}

void initializeFS()
{
  if (!LittleFS.begin(true))
  {
      Serial.println("[FS] LittleFS mount FAILED – halting");
      while (true)
        delay(1000);
  }
  Serial.println("[FS] LittleFS mounted OK");
  // List root directory to verify uploaded files
  File root = LittleFS.open("/");
  File file = root.openNextFile();
  while (file) {
    Serial.printf("[FS]  %-30s  %6u bytes\n", file.name(), file.size());
    file = root.openNextFile(); // not sure if we should be calling file.close after?
  }
}

// upload to LittleFS
void handleFileUpload()
{
  HTTPUpload& upload = server.upload(); // there's a single unique upload per request client. 
  // We might still need to verify that we are processing a single upload i.e. clear old upload progress if new upload detected
  // actually maybe not, because HTTPUploadStatus would return UPLOAD_FILE_ABORTED
  switch(upload.status)
  {
    case HTTPUploadStatus::UPLOAD_FILE_START:
    {
      uploadCompleted = false;
      String filename = upload.filename;
      if (!filename.startsWith("/")) {
        filename = "/" + filename;
      }
      uploadFile = LittleFS.open(filename, FILE_WRITE, true);
      Serial.printf("Starting file upload. File: %-30s %6u bytes\n", uploadFile.name(), uploadFile.size());
      Serial.printf("UPLOAD_FILE_START: Uploaded %6u out of %6u bytes\n", uploadFile.size(), upload.totalSize);
      break;
    }
    case HTTPUploadStatus::UPLOAD_FILE_WRITE:
    {
      // upload.buf - current buffer chunk
      // upload.currentsize - current buffer size in bytes
      uploadFile.write(upload.buf, upload.currentSize);
      uploadFile.flush();
      Serial.printf("UPLOAD_FILE_WRITE: Uploaded %6u out of %6u bytes\n", uploadFile.size(), upload.totalSize);
      break;
    }
    case HTTPUploadStatus::UPLOAD_FILE_END:
    {
      Serial.println("UPLOAD_FILE_END: Finished File upload");
      // Serial.printf("Uploaded %6u out of %6u bytes\n", uploadFile.size(), upload.totalSize);
      uploadFile.close();
      uploadCompleted = true;
      break;
    }
    case HTTPUploadStatus::UPLOAD_FILE_ABORTED:
    {
      uploadFile.close();
      Serial.println("UPLOAD_FILE_ABORTED: File Upload aborted.");
      // Serial.printf("Uploaded %6u out of %6u bytes\n", uploadFile.size(), upload.totalSize);
      break;
    }
  }
  // UPLOAD_FILE_START,
  // UPLOAD_FILE_WRITE,
  // UPLOAD_FILE_END,
  // UPLOAD_FILE_ABORTED
}

void setup()
{
  Serial.begin(115200);

  initializeFS();

  // setup OTA

  // setup wifi connection
  auto status = WiFi.begin(Ssid, Password);
  while(status != WL_CONNECTED)
  {
    delay(500);
    status = WiFi.status();
    Serial.print('.');
  }
  Serial.println();
  Serial.printf("Connected to Wifi: %s\n", WiFi.localIP().toString());
  startMDNS();

  server.on("/", []{
    server.send(200, "text/html; charset=utf-8", pageHome());
  });

  server.on("/fs-upload", HTTP_GET, []{
    server.send(200, "text/html; charset=utf-8", pageFSUpload());
  });

  server.on("/update", HTTP_GET, []{
    server.send(200, "text/html; charset=utf-8", pageUpdate());
  });

  // HTTP_POST actual FS file upload
  server.on("/fs-upload", HTTP_POST, []{
    if (uploadCompleted)
      server.send(200, "text/plain", "");
    else
      server.send(500, "text/plain", "");
  },
  handleFileUpload);

  // HTTP_POST actual firmware file upload
  server.on("/update", HTTP_POST, []{
    server.send(200, "text/plain", "");
  },
  []{
    Serial.println("Firmware update not yet implemented");
  });

  server.onNotFound([]{
    server.send(404, "text/html", pageNotFound());
  });

  server.begin();
}

void loop()
{
  server.handleClient();
  delay(2);
}
