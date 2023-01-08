#pragma once

#include <Arduino.h>
#include <WiFiClient.h>
#include <IPAddress.h>

#include <ArduinoJson.h>

extern StaticJsonDocument<3000> json_doc;

/// @class Config
/// @brief Provides configuration for network comms
struct Config
{
  public:
  IPAddress   ctrl_server_ip;     ///< Server IP address
  uint16_t    ctrl_server_port;   ///< Server port
  uint32_t    comms_timeout;      ///< Comms Timeout in seconds
}; // Config

extern const Config CTRL; ///< Global configuration instance.

bool http_get(String const& url, String const& body, String& data);

bool process_response(WiFiClient &client, String& data);

// Sends a set_temp request to the server for the given controller.
void send_controller_set_temp(String const& dev_id, float &set_temp);

// Gets the current value of the controller's set temperature
float get_controller_set_temp(String const& dev_id);

