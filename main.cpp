#include <iostream>
#include <unistd.h>
#include <sstream>
#include <cmath>
#include <wiringPi.h>
#include "dht22.h"


int main() {

    wiringPiSetup();
    DHT22 dht(0);
    std::vector<processed_data> historical;

    while (1) {

        int i = 0;
        while (i < 12) {
            sleep(5);
            processed_data misured_data = dht.misure();
            historical.push_back(misured_data);
            i++;
        }

        float temperature;
        try {

            dht.calcTempMedia(historical, temperature);
            temperature = round(temperature * 10) / 10; //round to first number after decimal

            if(temperature > 28.0) {

                std::cout << "ATTENZIONE: TEMPERATURA SOPRA LA NORMA!" << std::endl;
                std::cout << "VALORE TEMPERATURA : " << temperature << std::endl;

                //converti float in string mantenendo inalterato il numero di cifre dopo la virgola
                std::ostringstream os;
                os << temperature;
                dht.sendSMS(os.str(), "<SEND_TO_NUMBER>");
                dht.sendMail(os.str());
            }

        }catch(std::runtime_error &err) {
            std::cout << err.what() << std::endl;
            std::cout << "Prossimo tentativo in 60s..." << std::endl;
        }
        sleep(1800);
    }
    return 0;
}
