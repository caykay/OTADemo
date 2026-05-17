#pragma once

#include "esp_ota_ops.h"

#include <WebServer.h>

namespace FirmwareManager {
using OnComplete = std::function<void()>;
esp_err_t onWrite(const HTTPUploadStatus uploadStatus, uint8_t *buffer,
                  const size_t bufferSize, OnComplete onComplete = nullptr);
void reset();
} // namespace FirmwareManager
