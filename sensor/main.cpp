#include <mbed.h>

#include "HTS221Sensor.h"

#define PRODUCTION
   
DigitalOut led(LED1);
InterruptIn button(USER_BUTTON);
Thread t;
EventQueue queue(5 * EVENTS_EVENT_SIZE);
Serial pc(USBTX, USBRX);
WiFiInterface *wifi;

DevI2C i2c(PB_11, PB_10);

HTS221Sensor hts221(&i2c);

#if defined DEBUG

#include "http_request.h"
char* endpoint = "http://ictes.herokuapp.com;
#else
#include "https_request.h"
char* endpoint = "https://data.heroku.com/datastores/06079ce0-12fa-490e-a62d-a94b70da16ed";
const char SSL_CA_PEM[] ="-----BEGIN CERTIFICATE-----\n"
"MIIDRTCCAi2gAwIBAgIJQgAAASJeXTK7MA0GCSqGSIb3DQEBCwUAMFQxGTAXBgNV\n"
"BAoMEEFPIEthc3BlcnNreSBMYWIxNzA1BgNVBAMMLkthc3BlcnNreSBBbnRpLVZp\n"
"cnVzIFBlcnNvbmFsIFJvb3QgQ2VydGlmaWNhdGUwHhcNMTkwMzAzMTYyMjE5WhcN\n"
"MjEwMzAyMTYyMjE5WjAaMRgwFgYDVQQDEw9kYXRhLmhlcm9rdS5jb20wggEiMA0G\n"
"CSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQC4sO8sZF3akKDdVkSE7zCHgLTDylU8\n"
"F7MGS0S9eSnkS0QkwGWrE78dNOcHO5xOgbkJhJ14WI6oeku5Wp6b3cWpWMoIRys5\n"
"d8oc+dq/YWJLgK+21CSwmfuVFwu5ltQeAhedfRyUDUnGjDyN4hxABTnOMBw1fuu7\n"
"D/1DMD9gob5ud9ZgZj8UUgqXXCgWtm/owz2qJUCmBsJMre+PVrYxr7QoFAgHrBLm\n"
"lZ/Hboq0Y3wgavVGlxYfEWyAffWilTd2wups/8uZ31r8fJ8e2akwy9+1pl7Hnjg5\n"
"weklQpeUFTHgDRHysCisOmqUwocAVUpM/jgrMJq1iUsLEmR3VNLYDZzxAgMBAAGj\n"
"VDBSMAsGA1UdDwQEAwIFoDBDBgNVHREEPDA6hwQD5ba/gg9kYXRhLmhlcm9rdS5j\n"
"b22CIWhlcm9rdWRhdGEtZnJvbnRlbmQuaGVyb2t1YXBwLmNvbTANBgkqhkiG9w0B\n"
"AQsFAAOCAQEAO97gwsVm2eoiSzTlnSzriCa6fnwwvzxjdApUOnTMtgPQdmDDD0Md\n"
"dLhHEeNNrqnmFA6/AD5g3MqWbliUgaeYOZXpdFG6oqxPNgTWMIVRTeSI8/48Ohge\n"
"9i5RSNo51U2WVvkHQEjE9w7vPNIYemEcQgpy59zhP++XfaZ1ZVnDjLyviipwsLeI\n"
"zdzoX+qFLRzlFXFeZqheNlMMG316gDuSiBEnmJihv8bU2hna9UIzqVx/oNiKzQ7Z\n"
"ps9oTBCGp4gxadBe1YVbut8UONp+zGS5heO2lJYxf6D2kVWYhHv1tyvlJPBn4Oq0\n"
"bXksPx4GojUoX6gIMhpDOTpCT/BG5vlb6A==\n"
"-----END CERTIFICATE-----\n";
 #endif    

float humidity;
float temperature;

void pressed_handler() {

    pc.printf("\nConnecting to %s...\n", MBED_CONF_APP_WIFI_SSID);

    int ret = wifi->connect(MBED_CONF_APP_WIFI_SSID, MBED_CONF_APP_WIFI_PASSWORD, NSAPI_SECURITY_WPA_WPA2);
    if (ret != 0) {
        pc.printf("\nConnection error: %d\n", ret);
        return;
    }


    pc.printf("Success\n\n");
    printf("MAC: %s\n", wifi->get_mac_address());
    printf("IP: %s\n", wifi->get_ip_address());
    printf("Netmask: %s\n", wifi->get_netmask());
    printf("Gateway: %s\n", wifi->get_gateway());
    printf("RSSI: %d\n\n", wifi->get_rssi());   

    while(1) {

        hts221.enable();
        hts221.get_humidity(&humidity);
        hts221.get_temperature(&temperature);
        hts221.disable();
        hts221.reset();

        char body[128];
        sprintf(body,"{\"humidity\":\%.2f,\"temperature\":\%.2f}", humidity, temperature);
#if defined DEBUG
        pc.printf("[DEBUG] ");
        HttpRequest* request = new HttpRequest(wifi, HTTP_POST, endpoint);
#else
        HttpsRequest* request = new HttpsRequest(wifi, SSL_CA_PEM, HTTP_POST, endpoint);
#endif
        request->set_header("Content-Type", "application/json");
        HttpResponse* response = request->send(body, strlen(body));
        pc.printf("Humidity: %f\t Temperature: %f\t Status: %d\n\r", humidity, temperature, response->get_status_code());
        //printf("Status is %d - %s\n", response->get_status_code(), response->get_status_message());
        //printf("body is:\n%s\n", response->get_body());
        
        delete request;// also clears out the response

        //wifi->disconnect();
        //pc.printf("\nDone\n");    

        ThisThread::sleep_for(1000*60);
   }
   
}

int main() {

#if defined DEBUG
    pc.printf("[DEBUG]\n\r");
#endif

    wifi = WiFiInterface::get_default_instance();
    if (!wifi) {
        printf("ERROR: No WiFiInterface found.\n");
        return -1;
    }
    t.start(callback(&queue, &EventQueue::dispatch_forever));
    button.fall(queue.event(pressed_handler));
    pc.printf("Ready\n");

    if (hts221.init(NULL)!=0) {
        printf("Could not initializ HTS211\n\r");
        return -1;
    }

    while(1) {
        led = !led;
        ThisThread::sleep_for(500);
    }

}
