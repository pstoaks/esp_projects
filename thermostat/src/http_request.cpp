#include <Arduino.h>
#include <WiFiClient.h>
#include <IPAddress.h>

#include "timer.h"

#include "http_request.h"

// Allocate a temporary JsonDocument
// Don't forget to change the capacity to match your requirements.
// Use https://arduinojson.org/v6/assistant to compute the capacity.
StaticJsonDocument<3000> json_doc;

static constexpr char CrLf[] = "\r\n";

const Config CTRL = {
  "CtrlServer",                 ///< Device id
  IPAddress(192, 168, 0, 10),   ///< Server IP address
  8000U,                        ///< Server port
  3000                          ///< Comms Timeout in milliseconds
}; // CTRL global Config

// @todo These are constants now, but they should really be looked up in the
//    database on initialization.

// ESP_2255 Living room corner light.
const Config LAMP_1 = {
  "ESP_2255",                   ///< Device id
  IPAddress(192, 168, 0, 25),   ///< Server IP address
  80U,                          ///< Server port
  2000                          ///< Comms Timeout in milliseconds
}; // CTRL global Config

// ESP_B4E Family room clored lights
const Config LAMP_2 = {
  "ESP_84E",                    ///< Device id
  IPAddress(192, 168, 0, 24),   ///< Server IP address
  80U,                          ///< Server port
  2000                          ///< Comms Timeout in milliseconds
}; // CTRL global Config


static bool process_response(WiFiClient &client, String& data);

bool http_request(const IPAddress& ip_addr, unsigned port, const char* method,
  const char* url, const char* body, String& data)
{
  // Static shared HTTP request buffer. Note that only one HTTP request
  // can be in progress at any one time.
  static char HttpRequestBuf[2048];

  snprintf(HttpRequestBuf, sizeof(HttpRequestBuf),
    "%s %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: SSW_IOT Device\r\nAccept: application/json\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n%s\r\n",
    method, url, ip_addr.toString(), strlen(body), body);

  WiFiClient client;
  Serial.printf("HTTP_Request: %s\n\n", HttpRequestBuf);
  if ( !client.connect( ip_addr, port) )
  {
    Serial.println("Connection failed.\n");
    return false;
  }

  // If we have successfully connected, send our request:
  client.print(HttpRequestBuf);

  return process_response(client, data);

} // http_request()

bool http_get_from_server(const char* url, const char* body, String& data)
{

  return http_request(CTRL.ctrl_server_ip, CTRL.ctrl_server_port, "GET", url, body, data);

} // http_get_from_server()

bool process_response(WiFiClient &client, String& data)
{
  bool resp_ok; // Return value

  // Wait for response
  SSW::Timer respTimer(CTRL.comms_timeout);
  respTimer.reset();
  while ( !client.available() && !respTimer.expired() )
  {
    delay(500);
  }

  if ( !client.available() )
  {
    // Timed out
    Serial.println("WARNING: HTTP Request timed out!");
    return false;
  }
  else
  {
    // Serial.println("Response received.");

    // The reason we don't short circuit on a bad return code
    // is because we want to be able to log the entire response.
    while ( client.available() )
    {
      String line = client.readStringUntil('\r');
      line.trim();
      // Serial.println(line);
      // Check return code
      if (line.startsWith("HTTP"))
      {
        if (line.endsWith("200 OK"))
        {
          resp_ok = true;
        }
        else
        {
          resp_ok = false;
          Serial.println("Request Failed.  Return Code:");
          Serial.println(line);
          Serial.println();
        }
      }
      else if (resp_ok && line.length() < 2)
      {
        // next line should be the body.
        data = client.readStringUntil('\r');
        data.trim();
//        Serial.println(String("Response Body: ") + data);
      }
    }
  }

  return resp_ok;
} // process_response()

/// @brief Sends a new set_temp value to the server for the given controller
/// @param dev_id Controller ID
/// @param set_temp In/Out parameter. Gets the value returned by the server.
void send_controller_set_temp(const char* dev_id, float &set_temp)
{
    static const char URL_TEMPLATE[] {"/controller/set_temp?dev_id=%s&set_temp=%3.1f"};
    static constexpr unsigned int MAX_URL_LEN = sizeof(URL_TEMPLATE)+15;
    char get_url[MAX_URL_LEN] {""};
    String json;

    snprintf(get_url, sizeof(get_url), URL_TEMPLATE, dev_id, set_temp);

    if ( http_get_from_server(get_url, "", json) )
    {
      // Set the display and the position based on the return value.
      DeserializationError error = deserializeJson(json_doc, json);
      if ( error )
      {
        Serial.printf("JSON Decoding Error: %s\n", error.c_str());
      }
      else
      {
          set_temp = json_doc["subdevs"]["controller"]["state"]["set_temp"];
          // Serial.printf("Controller set temp: %3.1f\n", set_temp);
      }
    }
    else
      Serial.println("send_controller_set_temp() failed.\n");
} // send_controller_set_temp()

/// @brief Gets the controller's set temp
/// @param dev_id Controller ID
/// @return Returns the current set temperature or a negative number if
///    the server request is not succesful.
float get_controller_set_temp(const char* dev_id)
{
  float rtn = -1.0;
  if ( get_device_state(dev_id) )
  {
    rtn = json_doc["subdevs"]["controller"]["state"]["set_temp"];
    // Serial.printf("Controller set temp: %3.1f\n", rtn);
  }

  return rtn;
} // get_controller_set_temp()

/// @brief Queries the server for the given device and leaves the response in json_doc
/// @param dev_id : Device being queried for.
/// @return true for success
bool get_device_state(const char* dev_id)
{
  bool rtn = false;
  static const char URL_TEMPLATE[] {"/device?dev_id=%s"};
  char get_url[sizeof(URL_TEMPLATE)+15] {""};
  String json;

  snprintf(get_url, sizeof(get_url), URL_TEMPLATE, dev_id);
  // Serial.println(get_url);
  if ( http_get_from_server(get_url, "", json) )
  {
    DeserializationError error = deserializeJson(json_doc, json);
    if ( error )
    {
      Serial.printf("JSON Decoding Error: %s\n", error.c_str());
    }
    else
    {
      rtn = true;
    }
  }
  else
    Serial.printf("Request for %s failed.\n\n", dev_id);

  return rtn; // Data is in json_doc
} // get_device_state()

#if 0
// Not used. Attempts to speak directly to the relay. This doesn't work very well.
bool set_relay_state_direct(const Config& device, bool state)
{
  const char URL_TEMPLATE[] {"/relay_1/%s"};
  char url[sizeof(URL_TEMPLATE)+4];
  String data;
  snprintf(url, sizeof(url), URL_TEMPLATE, state ? "on" : "off");
  bool rtn = http_request(device.ctrl_server_ip, device.ctrl_server_port, "GET",
    url, "", data);

  // Nothing to do with data at this time.

  return rtn;
} // set_relay_state()
#endif

bool set_relay_state(const Config& device, bool state)
{
  // http://192.168.0.10:8000/relay/set_state?dev_id=ESP_2255&subdev=relay_1&state=on
  static const char URL_TEMPLATE[] {"/relay/set_state?dev_id=%s&subdev=%s&state=%d"};
  char url[sizeof(URL_TEMPLATE)+22] {""};
  String data;
  snprintf(url, sizeof(url), URL_TEMPLATE, device.dev_id, "relay_1", state ? 1 : 0);
  bool rtn = http_request(CTRL.ctrl_server_ip, CTRL.ctrl_server_port, "GET",
    url, "", data);

  if ( !rtn )
    Serial.println("set_relay_state() failed.");

  // Nothing to do with data at this time.

  return rtn;
} // set_relay_state()

bool get_relay_state(const Config& device, bool &state)
{
  bool rtn {false};
  if ( get_device_state(device.dev_id) )
  {
    int _state = json_doc["subdevs"]["relay_1"]["state"]["state"];
    Serial.printf("Relay state: %d\n\n", _state);
    state = (1 == _state );
    rtn = true;
  }
  return rtn;
} // get_relay_state()