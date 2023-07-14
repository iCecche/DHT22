//
// Created by ALESSIO CECCHERINI on 14/07/23.
//

#ifndef DHT22_DHT22_H
#define DHT22_DHT22_H


#include <cstdint>
#include <vector>
#include <wiringPi.h>
#include <string>

//dati forniti dal sensore
struct processed_data {

    float humidity;
    float temperature;

    //overload per assegnare tutte le variabili a -1 in caso di errore con singola istruzione
    processed_data& operator=(int const& num) {
        humidity = num;
        temperature = num;
        return *this;
    }
};

class DHT22 {
public:
    explicit DHT22(int pin);
    DHT22(const DHT22& sensor) = delete;

    struct processed_data misure();

    void calcTempMedia(std::vector<processed_data>& historical, float &temperature);
    bool sendSMS(std::string temperature, std::string sendTo);
    bool sendMail(std::string temperature);

private:

    void initPin() const;
    long int waitResponse(int state) const;
    void readData();
    int controlData(int hh, int lh, int ht, int lt);
    void print() const;

    int64_t data;
    processed_data misured_data{};
    int gpio;
};


#endif //DHT22_DHT22_H
