//
// Created by ALESSIO CECCHERINI on 14/07/23.
//

#include "dht22.h"
#include <iostream>
#include <unistd.h>
#include <chrono>
#include <curl/curl.h>
#include <wiringPi.h>

using namespace std::chrono;

DHT22::DHT22(int pin) : gpio(pin) {
    misured_data = -100;
    data = -1;
}

void DHT22::initPin() const {

    pinMode(gpio, OUTPUT);
    digitalWrite(gpio, LOW);

    usleep(20000);

    digitalWrite(gpio, HIGH);
    usleep(40);
    pinMode(gpio, INPUT);

}

struct processed_data DHT22::misure() {
    initPin();
    data = 0;
    try {
        waitResponse(1);
        waitResponse(0);
        for (int i = 0; i < 40; i++) {
            data <<= 1;
            waitResponse(0);
            int high_time = waitResponse(1);
            if (50 < high_time) {
                data |= 0x1;
            }
        }
        waitResponse(0);
        readData();
    }catch(...) {
        std::cout << "Errore nella misurazione..." << std::endl;
        misured_data = -100;
        pinMode(gpio, OUTPUT);
        digitalWrite(gpio, LOW);
    }
    pinMode(gpio, OUTPUT);
    digitalWrite(gpio, HIGH);
    return misured_data;
}

long int DHT22::waitResponse(int state) const {

    struct timespec begin, end;
    clock_gettime(CLOCK_REALTIME, &begin);
    int count = 10000;

    while (digitalRead(gpio) == state){
        if(count == 0) {
            throw std::runtime_error("Timeout while waiting for pin to get response (low or high)");
        }
        count--;
    }

    clock_gettime(CLOCK_REALTIME, &end);
    return (end.tv_nsec - begin.tv_nsec) / 1000;
}

void DHT22::readData() {

    unsigned int humidity = (data >> 32) & 0xFF;
    unsigned int decimal_hum = (data >> 24) & 0xFF;
    unsigned int temperature = (data >> 16) & 0xFF;
    unsigned int decimal_temp = (data >> 8) & 0xFF;
    unsigned int parity = data & 0xFF;

    if (parity == controlData(humidity,decimal_hum,temperature, decimal_temp)){

        humidity*= 0x100; // shift >> 8 position to add decimal part
        humidity += decimal_hum;
        temperature *= 0x100; // shift >> 8 position to add decimal part
        temperature += decimal_temp;

        misured_data.temperature = static_cast<float>(temperature) / 10;
        misured_data.humidity = static_cast<float>(humidity) / 10;

        if ((data >> 16) & 0x80) { //check for negative temperature
            misured_data.temperature *= -1;
        }

        print();

    }else {
        std::cout << "Data corrupted!" << std::endl;
        misured_data = -100;
    }
}

int DHT22::controlData(int hh, int lh, int ht, int lt) {
    return static_cast<int>(hh + ht + lh + lt);
}

void DHT22::print() const {
    std::cout << "Nuova misurazione in corso..." << std::endl;
    std::cout << "Temp: " << misured_data.temperature << " °C" <<std::endl;
    std::cout << "Hum: " << misured_data.humidity << " %" << std::endl;
}

void DHT22::calcTempMedia(std::vector<processed_data> &historical, float &temperature) {

    float sumTemp = 0;
    int valid_record = 0;

    for(auto record : historical) {
        if (record.temperature != -100) {
            sumTemp += record.temperature;
            valid_record++;
        }
    }

    temperature = sumTemp / static_cast<float>(valid_record);

    if (temperature == 0) {
        throw std::runtime_error("Error calculating temperature average!");
    }
}

bool DHT22::sendSMS(std::string temperature, std::string sendTo) {

    CURL* curl;
    CURLcode res;

    std::string url = "https://graph.facebook.com/v17.0/<YOUR_ID_NUMBER>/messages";
    char* accessToken = "Authorization: Bearer <YOUR_ACCESS_TOKEN>";
    std::string jsonData = R"({ "messaging_product": "whatsapp", "to": )" + sendTo + R"(, "type": "text", "text": { "preview_url": false, "body": "Alert! Temperature is: )" +
                           temperature + R"( C" } } )";
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, accessToken);
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonData.c_str());

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "Error performing POST request: " << curl_easy_strerror(res) << std::endl;
            throw std::runtime_error("Error performing POST request during Whatsapp send message action");
        }
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
    return true;
}


bool DHT22::sendMail(std::string temperature) {

    CURL *curl;
    CURLcode res;
    curl_mime *mime1;
    curl_mimepart *part1;

    mime1 = nullptr;

    temperature = "Temperature is: " + temperature + " °C";
    auto mess = temperature.c_str();

    curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 102400L);
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.mailgun.net/v3/<YOUR_DOMAIN_NAME>/messages");
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(curl, CURLOPT_USERPWD, "api:<API_KEY>");
        mime1 = curl_mime_init(curl);
        part1 = curl_mime_addpart(mime1);
        curl_mime_data(part1, "Temperature Station <mailgun@<YOUR_DOMAIN_NAME>>", CURL_ZERO_TERMINATED);
        curl_mime_name(part1, "from");
        part1 = curl_mime_addpart(mime1);
        curl_mime_data(part1, "<TO_EMAIL_ADDRESS>", CURL_ZERO_TERMINATED);
        curl_mime_name(part1, "to");
        part1 = curl_mime_addpart(mime1);
        curl_mime_data(part1, "Temperature Alert!", CURL_ZERO_TERMINATED);
        curl_mime_name(part1, "subject");
        part1 = curl_mime_addpart(mime1);
        curl_mime_data(part1, mess , CURL_ZERO_TERMINATED);
        curl_mime_name(part1, "text");
        curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime1);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/7.87.0");
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
        curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_2TLS);
        curl_easy_setopt(curl, CURLOPT_FTP_SKIP_PASV_IP, 1L);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
    }else {
        throw std::runtime_error("Error sending email with mailgun api!");
    }

    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        std::cerr << "Error: " <<curl_easy_strerror(res) << std::endl;
        return false;
    }

    curl_easy_cleanup(curl);
    curl_mime_free(mime1);
    curl_global_cleanup();
    return true;
}