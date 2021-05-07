#include <EspWifiManager.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

#include <ArxTypeTraits.h>

#define WEB_SERVER_PORT    80
#define MAX_PAYLOAD_LEN    8
#define MAX_JSON_CONN_SIZE 150
#define JSON_SIZE          64
#define PUT_JSON_SIZE      64
#define MAX_SERVICE_LEN    16
#define MAX_STRMSG_SIZE    8


template<typename T>
using SensorCallback = T (*)();

template<typename T>
using ActuatorCallback = void (*)(T);


/*
    Sensor/Actuator APIs
*/

template<typename T>
class MoodySensor {
    private:
        const char* serviceName;
        AsyncWebServer sensorServer;
        SensorCallback<T> sensorCallback;

    public:
        MoodySensor(String serviceName);
        MoodySensor(const char* serviceName);
        void setAcquireFunction(SensorCallback<T> callback);
        void begin();
        void loop();
        virtual ~MoodySensor() {}
};


template<typename T>
class MoodyActuator {
    private:
        const char* actuatorIdentifier;
        AsyncWebServer actuatorServer;
        ActuatorCallback<T> actuatorCallback;

    public:
        MoodyActuator(String actuatorIdentifier);
        MoodyActuator(const char* actuatorIdentifier);
        void setActuateFunction(ActuatorCallback<T> callback);
        void begin();
        void loop();
        virtual ~MoodyActuator() {}
};


/*
    Sensor implementation
*/


template<typename T>
MoodySensor<T>::MoodySensor(String serviceName) : sensorServer(WEB_SERVER_PORT), sensorCallback(nullptr), serviceName(serviceName.c_str())
{
    static_assert(std::is_arithmetic<T>::value, "T can only be a number");
}


template<typename T>
MoodySensor<T>::MoodySensor(const char* serviceName) : sensorServer(WEB_SERVER_PORT), sensorCallback(nullptr), serviceName(serviceName)
{
    static_assert(std::is_arithmetic<T>::value, "T can only be a number");
}


template<typename T>
void MoodySensor<T>::setAcquireFunction(SensorCallback<T> callback)
{
    sensorCallback = callback;
}


template<typename T>
void MoodySensor<T>::begin()
{
    sensorServer.on("/api/conn", HTTP_GET, [this](AsyncWebServerRequest *request) {
        String resp;
        String mac = WiFi.macAddress();
        StaticJsonDocument<MAX_JSON_CONN_SIZE> jsonDoc;

        jsonDoc["type"] = "sensor";
        jsonDoc["service"] = serviceName;
        jsonDoc["mac"] = mac;
        serializeJson(jsonDoc, resp);

        request->send(200, "application/json", resp);
    });

    sensorServer.on("/api/data", HTTP_GET, [this](AsyncWebServerRequest *request) {
        String resp;
        StaticJsonDocument<JSON_SIZE> jsonDoc;
        if(this->sensorCallback)
        {
            T sensorData = this->sensorCallback();
            jsonDoc["payload"] = sensorData;
        }
        else
        {
            jsonDoc["payload"] = nullptr;
        }
        serializeJson(jsonDoc, resp);
        request->send(200, "application/json", resp);
    });

    sensorServer.begin();
}


template<typename T>
void MoodySensor<T>::loop()
{
    WiFiManager.loop();
}


/*
    Actuator implementation
*/

template<typename T>
MoodyActuator<T>::MoodyActuator(String actuatorIdentifier) : actuatorServer(WEB_SERVER_PORT), actuatorCallback(nullptr), actuatorIdentifier(actuatorIdentifier.c_str())
{
    static_assert(std::is_arithmetic<T>::value, "T can only be a number");
}

template<typename T>
MoodyActuator<T>::MoodyActuator(const char* actuatorIdentifier) : actuatorServer(WEB_SERVER_PORT), actuatorCallback(nullptr), actuatorIdentifier(actuatorIdentifier)
{
    static_assert(std::is_arithmetic<T>::value, "T can only be a number");
}

template<typename T>
void MoodyActuator<T>::setActuateFunction(ActuatorCallback<T> callback)
{
    actuatorCallback = callback;
}

template<typename T>
void MoodyActuator<T>::begin()
{
    actuatorServer.on("/api/conn", HTTP_GET, [](AsyncWebServerRequest *request) {
        String resp;
        String mac = WiFi.macAddress();
        StaticJsonDocument<MAX_JSON_CONN_SIZE> jsonDoc;

        jsonDoc["type"] = "actuator";
        jsonDoc["mac"] = mac;
        serializeJson(jsonDoc, resp);

        request->send(200, "application/json", resp);
    });

    actuatorServer.on("/api/data", HTTP_PUT, [this](AsyncWebServerRequest *request, JsonVariant &json) {
        JsonObject& jsonObj = json.as<JsonObject>();
        if(jsonObj.containsKey("payload"))
        {
            T payload = jsonObj["payload"].as<T>();
            actuatorCallback(payload);
        }
    });

    actuatorServer.begin();
}

template<typename T>
void MoodyActuator<T>::loop()
{
    WiFiManager.loop();
}
