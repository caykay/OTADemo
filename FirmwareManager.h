#pragma once

#include "esp_ota_ops.h"

#include <WebServer.h>

namespace FirmwareManager {
esp_err_t onWrite(const HTTPUploadStatus uploadStatus, uint8_t *buffer,
                  const size_t bufferSize);
void reset();
} // namespace FirmwareManager
