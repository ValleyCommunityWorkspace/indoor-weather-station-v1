#include "CaptiveConfig.h"

// for network_selector_first and network_selector_second
#include "static/network-selector.h"

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>

#include <cassert>

CaptiveConfig * CaptiveConfig::instance(nullptr);

CaptiveConfig::CaptiveConfig() :
    configHTTPServer(nullptr),
    configDNSServer(nullptr),
    pickedCreds(nullptr),
    state(CaptiveConfigState::START_SCANNING),
    numAPsFound(0)
{
    assert(instance == nullptr);
    instance = this;
}


CaptiveConfig::~CaptiveConfig()
{
    assert(instance == this);

    configHTTPServer->close();
    delete configHTTPServer;
    configHTTPServer = nullptr;

    configDNSServer->stop();
    delete configDNSServer;
    configDNSServer = nullptr;

    if (pickedCreds) {
        delete pickedCreds;
    }

    tearDownKnownAPs();

    instance = nullptr;
}


bool CaptiveConfig::haveConfig()
{
    switch (state) {
        case CaptiveConfigState::START_SCANNING:
            // Set WiFi to station mode and disconnect from an AP, in case
            // it was previously connected.
            // TODO: This seems to be covered in scanNetworks - double check in the rescanning case
            WiFi.mode(WIFI_STA);
            WiFi.disconnect();

            // Scan asynchronously, don't show hidden networks.
            WiFi.scanNetworks(true, false);

            state = CaptiveConfigState::SCANNING;
            return false;

        case CaptiveConfigState::SCANNING:
        {
            auto scanState(WiFi.scanComplete());

            if (scanState == WIFI_SCAN_RUNNING) {
                return false;
            } else if (scanState == WIFI_SCAN_FAILED) {
                state = CaptiveConfigState::START_SCANNING;
                return false;
            }

            populateKnownAPs(scanState);

            state = CaptiveConfigState::STARTING_WIFI;
            return false;
        }

        case CaptiveConfigState::STARTING_WIFI:
            WiFi.mode(WIFI_AP);
            WiFi.softAPConfig( captiveServeIP, captiveServeIP,
                               IPAddress(255, 255, 255, 0) );
            WiFi.softAP(APNAME);

            // If this is our first time starting up (ie not a rescan)
            if (configHTTPServer == nullptr) {
                state = CaptiveConfigState::STARTING_HTTP;
            } else {
                state = CaptiveConfigState::SERVING;
            }
            return false;

        case CaptiveConfigState::STARTING_HTTP:
            configHTTPServer = new ESP8266WebServer(80);
            configHTTPServer->on("/getAPs", serveApJson);
            configHTTPServer->on("/storePassword", storePassword);
            configHTTPServer->on("/", serveConfigPage);
            configHTTPServer->onNotFound(serveRedirect);
            configHTTPServer->begin();

            state = CaptiveConfigState::STARTING_DNS;
            return false;

        case CaptiveConfigState::STARTING_DNS:
            configDNSServer = new DNSServer;
            configDNSServer->setTTL(0);
            configDNSServer->start(53, "*", captiveServeIP);

            state = CaptiveConfigState::SERVING;
            return false;

        case CaptiveConfigState::SERVING:
            configDNSServer->processNextRequest();
            configHTTPServer->handleClient();

            // storePassword() advances state to DONE
            return false;

        case CaptiveConfigState::DONE:
            return true;

        default:
            assert(0);
            return false;
    }
}


APCredentials CaptiveConfig::getConfig() const
{
    assert(pickedCreds);
    assert(state == CaptiveConfigState::DONE);

    if (pickedCreds) {
        return *pickedCreds;
    }

    return APCredentials();
}


void CaptiveConfig::populateKnownAPs(uint8_t numAPs)
{
    tearDownKnownAPs();

    knownAPs = new APType *[numAPs];
    for(auto i(0); i < numAPs; ++i) {
        knownAPs[i] = nullptr;

        auto thisSSID( WiFi.SSID(i) );
        auto thisRSSI( WiFi.RSSI(i) );

        auto isNewSSID(true);
        for(auto j(0); j < numAPsFound; ++j) {
            if( knownAPs[j]->ssid == thisSSID ) {
                isNewSSID = false;
                if( knownAPs[j]->rssi > thisRSSI ) {
                    knownAPs[j]->ssid = thisSSID;
                    knownAPs[j]->rssi = thisRSSI;
                    knownAPs[j]->encryptionType = WiFi.encryptionType(i);
                }
                break;
            }
        }

        if(isNewSSID) {
            knownAPs[numAPsFound] = new APType{ thisSSID,
                                                thisRSSI,
                                                WiFi.encryptionType(i) };
            ++numAPsFound;
        }
    }
}


void CaptiveConfig::tearDownKnownAPs()
{
    auto numToTearDown(numAPsFound);
    numAPsFound = 0;
    
    for(auto i(0); i < numToTearDown; ++i) {
        if(knownAPs[i] != nullptr) {
            delete knownAPs[i];
            knownAPs[i] = nullptr;
        }
    }

    delete [] knownAPs;

    knownAPs = nullptr;
}


String CaptiveConfig::makeApJson() const
{
    String out("[");

    for(auto i(0); i < numAPsFound; ++i) {
        if(i) {
            out += ",";
        }
        out += "{\n\"ssid\": \"";
        out += knownAPs[i]->ssid;
        out += "\",\n\"rssi\": ";
        out += knownAPs[i]->rssi;
        out += ",\n\"encryptionType\": \"";

        switch( knownAPs[i]->encryptionType ) {
            case ENC_TYPE_TKIP:
                out += "TKIP";
                break;

            case ENC_TYPE_CCMP:
                out += "CCMP";
                break;

            case ENC_TYPE_WEP:
                out += "WEP";
                break;

            case ENC_TYPE_NONE:
                break;

            case ENC_TYPE_AUTO:
                out += "Auto";
                break;

            default:
                Serial.print("Got unknown enum for encryptionType: ");
                Serial.println(knownAPs[i]->encryptionType);
                out += "???";
                break;
        }
        out += "\"\n}\n";
    }
    out += "]";

    return out;
}


/*static*/ void CaptiveConfig::serveApJson()
{
    Serial.println("Serving JSON");
    instance->configHTTPServer->send( 200, "application/json",
                                      instance->makeApJson() );
}


/*static*/ void CaptiveConfig::storePassword()
{
    assert(instance && instance->configHTTPServer);

    char out[]{
        "<!doctype html>"
        "<html class=\"no-js\" lang=\"en\">"
        "Thanks!"
        "</html>"
        };

    instance->pickedCreds = new APCredentials{
        instance->configHTTPServer->arg("ssid"),
        instance->configHTTPServer->arg("pass")
        };

    instance->state = CaptiveConfigState::DONE;

    instance->configHTTPServer->send(200, "text/html", out);
}


/*static*/ void CaptiveConfig::serveConfigPage()
{
    Serial.println("Serving config page");

    assert(instance && instance->configHTTPServer);

    // network_selector is generated from HTML by the Makefile
    // in static/ and turned in to a header file.
    String out(network_selector);

    instance->configHTTPServer->send(200, "text/html", out);
}


/*static*/ void CaptiveConfig::serveRedirect()
{
    // This only sends one response, with a redirect header and no content.
    instance->configHTTPServer->sendHeader( "Location",
                                            "http://setup/",
                                            true );
    instance->configHTTPServer->send(302, "text/plain", "");
}

