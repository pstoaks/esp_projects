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
  const char* dev_id;             ///< Device name
  IPAddress   ctrl_server_ip;     ///< Server IP address
  uint16_t    ctrl_server_port;   ///< Server port
  uint32_t    comms_timeout;      ///< Comms Timeout in seconds
}; // Config

extern const Config CTRL; ///< Global configuration instance.
extern const Config LAMP_1; ///< Lamp 1 is the corner living room lamp
extern const Config LAMP_2; ///< Lamp 2 is the family room colored lights.

/// @brief Makes an HTTP request of a server and collects the response.
/// @param ip_addr : The IP address of the server.
/// @param port : The port on the server
/// @param method : The method (e.g. GET, POST, etc.)
/// @param url : The URL on the server
/// @param body : The body (JSON data) to be sent with the request
/// @param data : OUT: The data returned from the request.
/// @return true if successful
bool http_request(const IPAddress& ip_addr, unsigned port, const char* method,
  const char* url, const char* body, String& data);

/// @brief HTTP GET request to automation control server.
/// @param url : The URL on the server
/// @param body : The body (JSON data) to be sent with the request
/// @param data : OUT: The data returned from the request.
/// @return true if successful
bool http_get_from_server(const char* url, const char* body, String& data);

/// @brief Queries the server for the given device and leaves the response in json_doc
/// @param dev_id : Device being queried for.
/// @return true for success
bool get_device_state(const char* dev_id);

/// @brief Sends a new set_temp value to the server for the given controller
/// @param dev_id Controller ID
/// @param set_temp In/Out parameter. Gets the value returned by the server.
void send_controller_set_temp(const char* dev_id, float &set_temp);

/// @brief Gets the controller's set temp
/// @param dev_id Controller ID
/// @return Returns the current set temperature or a negative number if
///    the server request is not succesful.
float get_controller_set_temp(const char* dev_id);

/// @brief Sets the relay state for the given device
/// @param device Config for the device.
/// @param state ON if true, OFF if false
/// @return Returns true on success
bool set_relay_state(const Config& device, bool state);

/// @brief Returns the relay state
/// @param device: Config for the device
/// @param state:  Out parameter providing the state. Only valid if return is true
/// @return true for success, false for failure
bool get_relay_state(const Config& device, bool &state);



