#include <Arduino.h>
#include <WiFiClient.h>
#include <IPAddress.h>

#include "timer.h"

#include "http_request.h"

static constexpr char CrLf[] = "\r\n";

const Config CTRL = {
  IPAddress(192, 168, 0, 10),   ///< Server IP address
  8000U,                        ///< Server port
  2                             ///< Comms Timeout in seconds
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
  while (!client.available() && !respTimer.expired())
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
