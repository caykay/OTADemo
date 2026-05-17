// Attempt to setup a basic WiFi LAN OTA - uses the BasicOTA example from https://github.com/caykay/arduino-esp32/tree/master/libraries/ArduinoOTA/examples/BasicOTA

#include <WebServer.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <LittleFS.h>

#include "views.h"
#include "FirmwareManager.h"

static WebServer server(80);
constexpr char* Hostname = "esp32";
constexpr char* Ssid = "****";
constexpr char* Password = "****";

static File uploadFile; // file to be uploaded to LittleFS
static bool uploadCompleted = false;

// firmware upload state
struct FirmwareUploadStatus
{
  bool success = true;
  bool started = false;
  String error = "";
  void reset()
  {
    success = true;
    started = false;
    error = "";
    FirmwareManager::reset();
  }
};
static FirmwareUploadStatus firmwareUploadResult;

String formatPath(const String& path)
{
  String result = path;
  if (!result.startsWith("/")) {
    result = "/" + result;
  }
  return result;
}

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
      String filename = formatPath(upload.filename);
      uploadFile = LittleFS.open(filename, FILE_WRITE, true);
      Serial.printf("handleFileUpload(): Starting file upload. File: %-30s %6u bytes\n", uploadFile.name(), uploadFile.size());
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

void handleDelete()
{
  if (server.args() == 0) {
    // return fail
    server.send(400, "text/plain", "Invalid filename");
    return;
  }
  String path = formatPath(server.arg(0));
  Serial.printf("handleDelete(): Deleting %s ...\n", path);
  if (path == "/" || !LittleFS.exists((char *)path.c_str())) {
    // return fail
    Serial.println("handleDelete(): File does not exist");
    server.send(404, "text/plain", "Resource does not exist");
    return;
  }
  // do the actual delete
  if (!LittleFS.remove(path))
  {
    Serial.printf("handleDelete(): Failed to delete file: %s\n", path);
    server.send(404, "text/plain", "Delete failed");
    return;
  }
  // return OK
  server.send(200, "text/plain", "");
}

void handleFileList()
{
  // check if client HTTP request header has "application/json"
  if(server.hasHeader("Accept") && server.header("Accept").indexOf("application/json") >= 0)
  {
    String json = "[";
    File root = LittleFS.open("/");
    File file = root.openNextFile();
    while (file)
    {
      // if json already has a file entry
      if (json.length() > 1)
        json += ",";
      json += R"({"name":")" + String(file.name()) + R"(","size":)" + file.size() + R"(})";
      file = root.openNextFile(); // not sure if we should be calling file.close after?
    }
    json += "]";
    server.send(200, "application/json", json);
    return;
  }
  server.send(200, "text/html", pageFSFiles());
}

void handleFirmwareUpdate()
{
  firmwareUploadResult.started = true;
  HTTPUpload& upload = server.upload();
  // ensure we only process the firmware bytes when previous ota steps had ESP_OK result
  if(firmwareUploadResult.success)
  {
    esp_err_t result = FirmwareManager::onWrite(upload.status, upload.buf, upload.currentSize);
    if (result != ESP_OK)
    {
      firmwareUploadResult.success = false;
    }
  }
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
  Serial.printf("[WiFi] Connected to Wifi: %s\n", WiFi.localIP().toString());

  startMDNS();

  server.on("/", []{
    server.send(200, "text/html; charset=utf-8", pageHome());
  });

  server.on("/update", HTTP_GET, []{
    server.send(200, "text/html; charset=utf-8", pageUpdate());
  });

  server.on("/files", HTTP_GET, handleFileList);

  server.on("/delete", HTTP_DELETE, handleDelete);

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
    if (firmwareUploadResult.started && firmwareUploadResult.success)
      server.send(200, "text/plain", "");
    else
      server.send(404, "text/plain", "firmware upload failed");
    firmwareUploadResult.reset();
  }, handleFirmwareUpdate);

  // server.on("/info", HTTP_GET, []{
  //   server.send(200, "text/plain", "Congratz this is the new firmware");
  // });

  server.onNotFound([]{
    server.send(404, "text/html", pageNotFound());
  });

  // add "Accept" header to the collected headers so the server can explicitly handle
  // some client requested response content i.e. /files can either return html or json
  // depending on the client request http header accept type

  const char* headerKeys[] = { "Accept" };
  server.collectHeaders(headerKeys, 1);

  server.begin();
}

void loop()
{
  server.handleClient();
  delay(2);
}
