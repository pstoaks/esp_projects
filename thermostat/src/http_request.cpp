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
  IPAddress(192, 168, 0, 10),   ///< Server IP address
  8000U,                        ///< Server port
  2000                          ///< Comms Timeout in milliseconds
}; // CTRL global Config


bool http_get(String const& url, String const& body, String& data)
{
   String httpr =
      "GET " + url + " HTTP/1.1" + CrLf +
      "Host: " + CTRL.ctrl_server_ip.toString() + CrLf +
      "User-Agent: SSW_IOT Device" + CrLf +
      "Accept: application/json" + CrLf +
      "Content-Type: application/json" + CrLf +
      "Content-Length: " + String(body.length()) + CrLf +
      CrLf +
      body;

    WiFiClient client;
    Serial.println("Connecting to control server.");
    Serial.println(String("Connecting to ")+CTRL.ctrl_server_ip.toString()+String(":")+String(CTRL.ctrl_server_port));
    if ( !client.connect( CTRL.ctrl_server_ip, CTRL.ctrl_server_port) )
    {
        Serial.println("Connection failed.");
        return false;
    }
    
    // If we have successfully connected, send our request:
    client.print(httpr);

    return process_response(client, data);

} // get()

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

  if (!client.available())
  {
    // Timed out
    Serial.println("WARNING: Post to server timed out!");
    return false;
  }
  else
  {
    Serial.println("Response received.");

    // The reason we don't short circuit on a bad return code
    // is because we want to be able to log the entire response.
    while (client.available())
    {
      String line = client.readStringUntil('\r');
      line.trim();
      Serial.println(line);
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
        }
      }
      else if (resp_ok && line.length() < 2)
      {
        // next line should be the body.
        data = client.readStringUntil('\r');
        data.trim();
        Serial.println(String("Response Body: ") + data);
      }
    }
  }

  return resp_ok;
}

/// @brief Sends a new set_temp value to the server for the given controller
/// @param dev_id Controller ID
/// @param set_temp In/Out parameter. Gets the value returned by the server.
void send_controller_set_temp(String const& dev_id, float &set_temp)
{
    static const char URL_TEMPLATE[] {"/controller/set_temp?dev_id=%s&set_temp=%3.1f"};
    static constexpr unsigned int MAX_URL_LEN = sizeof(URL_TEMPLATE)+15;
    char get_url[MAX_URL_LEN] {""};
    String json;

    snprintf(get_url, MAX_URL_LEN, URL_TEMPLATE, dev_id.c_str(), set_temp);
    Serial.println(get_url);

    if ( http_get(get_url, "", json) )
    {
//      Serial.println("\n\nLiving room controller: " + json);

      // Set the display and the position based on the return value.
      DeserializationError error = deserializeJson(json_doc, json);
      if ( error )
      {
        Serial.printf("JSON Decoding Error: %s\n", error.c_str());
      }
      else
      {
          set_temp = json_doc["subdevs"]["controller"]["state"]["set_temp"];
          Serial.printf("Living room set temp: %3.1f\n", set_temp);
      }
    }
    else
      Serial.println("send_controller_set_temp() failed.");
} // send_controller_set_temp()

/// @brief Gets the controller's set temp
/// @param dev_id Controller ID
/// @return Returns the current set temperature.
float get_controller_set_temp(String const& dev_id)
{
    String json;
    float rtn = 0.0;
    if ( http_get("/device?dev_id=lr_temp", "", json) )
    {
//      Serial.println("\n\nLiving room controller: " + json);

      DeserializationError error = deserializeJson(json_doc, json);
      if ( error )
      {
        Serial.printf("JSON Decoding Error: %s\n", error.c_str());
      }
      else
      {
          rtn = json_doc["subdevs"]["controller"]["state"]["set_temp"];
          Serial.printf("Living room set temp: %3.1f\n", rtn);
      }
    }
    else
      Serial.println("Get living room set temperature failed.");

    return rtn;
} // get_controller_set_temp()

