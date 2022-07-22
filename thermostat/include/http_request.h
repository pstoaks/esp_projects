#pragma once

#include <Arduino.h>
#include <WiFiClient.h>
#include <IPAddress.h>

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
