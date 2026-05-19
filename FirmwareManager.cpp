#include "FirmwareManager.h"

namespace
{
const esp_partition_t* updatePartition = nullptr;
esp_ota_handle_t updateHandle = 0;

static void abort(const char* message)
{
  Serial.println(message);
  esp_ota_abort(updateHandle);
}
}  // namespace

namespace FirmwareManager
{
esp_err_t onWrite(const HTTPUploadStatus uploadStatus, uint8_t* buffer,
                  const size_t bufferSize)
{
  const esp_partition_t* bootPartition = esp_ota_get_boot_partition();
  const esp_partition_t* runningPartition = esp_ota_get_running_partition();
  if (bootPartition != runningPartition)
  {
    Serial.printf(
        "Configured OTA boot partition at offset 0x%08x, but running "
        "from offset 0x%08x",
        bootPartition->address, runningPartition->address);
    Serial.println(
        "(This can happen if either the OTA boot data or preferred "
        "boot image become corrupted somehow.)");
    return ESP_ERR_NOT_SUPPORTED;
  }
  switch (uploadStatus)
  {
    case UPLOAD_FILE_START:
    {
      Serial.println("[OTA] Firmware Upload START.");
      updatePartition = esp_ota_get_next_update_partition(nullptr);
      if (!updatePartition)
      {
        Serial.println(
            "[OTA] esp_ota_get_next_update_partition: Could not "
            "configure firmware partition.");
        return ESP_ERR_NOT_FOUND;
      }
      // size_t chunkSize = sizeof(uint8_t) * HTTP_UPLOAD_BUFLEN;
      esp_err_t result =
          esp_ota_begin(updatePartition, OTA_SIZE_UNKNOWN, &updateHandle);
      if (result != ESP_OK)
      {
        abort("[OTA] esp_ota_begin: failed to begin firmware update service.");
      }
      return result;
    }
    case UPLOAD_FILE_WRITE:
    {
      esp_err_t result = esp_ota_write(updateHandle, buffer, bufferSize);
      if (result != ESP_OK)
      {
        abort(
            "[OTA] esp_ota_write: could not write ota firmware data to "
            "ota partition.");
      }
      return result;
    }
    case UPLOAD_FILE_END:
    {
      Serial.println("[OTA] Firmware Upload WRAPPING UP.");
      esp_err_t result = esp_ota_end(updateHandle);
      if (result != ESP_OK)
      {
        abort(
            "[OTA] esp_ota_end: could not finalize firmware update, "
            "likely that firmware image is invalid");
      }
      else
      {
        result = esp_ota_set_boot_partition(updatePartition);
        if (result != ESP_OK)
        {
          abort(
              "[OTA] esp_ota_set_boot_partition: could not set the "
              "firmware partition as the boot partition");
        }
        else
        {
          Serial.println("[OTA] Firmware Upload COMPLETE, restarting board");
          // esp_restart();
        }
      }
      return result;
    }
    case UPLOAD_FILE_ABORTED:
    default:
      return ESP_ERR_NOT_SUPPORTED;
  }
}

void reset()
{
  updatePartition = nullptr;
  updateHandle = 0;
}
}  // namespace FirmwareManager
