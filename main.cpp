/*
================================================================================
    Author: Bhagi Dhakal
    BlockHouse: Trial Task 1: Order Flow Imbalances (OFI)
    Description: Construction of the following OFI features, Best-Level OFI,
        Multi-Level OFI, Integrated OFI and Cross-Asset OFI

    File Name: main.cpp
    Compile: g++ -std=c++20 main.cpp -o main
*/

/* Includes */
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

/* Order Book Snapshot */
struct orderbookSnapshot {
    std::string time_stamp;
    double bid_px_00;
    double ask_px_00;
    int bid_sz_00;
    int ask_sz_00;
};

/* Bid and Ask logic */
double bidLogic(const orderbookSnapshot& current, const orderbookSnapshot& previous) {
    if (current.bid_px_00 > previous.bid_px_00) {
        return current.bid_sz_00;
    }
    else if (current.bid_px_00 == previous.bid_px_00) {
        return current.bid_sz_00 - previous.bid_sz_00;
    }
    else {
        // current.bid_px_00 < previous.bid_px_00
        return -current.bid_sz_00;
    }
}

double askLogic(const orderbookSnapshot& current, const orderbookSnapshot& previous) {
    if (current.ask_px_00 > previous.ask_px_00) {
        return -current.ask_sz_00;
    }
    else if (current.ask_px_00 == previous.ask_px_00) {
        return current.ask_sz_00 - previous.ask_sz_00;
    }
    else {
        // current.ask_px_00 < previous.ask_px_00
        return current.ask_sz_00;
    }
}


/* Construction of Best-Level OFI */
class BestLevelOFI {
public:
    double compute(const orderbookSnapshot& current, const orderbookSnapshot& previous) {
        double bid_size = bidLogic(current, previous);
        double ask_size = askLogic(current, previous);

        return bid_size - ask_size;
    }
};

/* File Line Parser to orderbookSnapshot */
orderbookSnapshot parseLineToSnapshot(const std::string& line) {
    std::stringstream ss(line);
    std::string token;
    std::vector<std::string> fields;

    // Reads each, value seperated by , in the line
    while (getline(ss, token, ',')) {
        fields.push_back(token);
    }

    orderbookSnapshot snapshot;
    snapshot.time_stamp = fields[0];
    snapshot.bid_px_00 = std::stod(fields[13]);
    snapshot.ask_px_00 = std::stod(fields[14]);
    snapshot.bid_sz_00 = std::stod(fields[15]);
    snapshot.ask_sz_00 = std::stod(fields[16]);

    //std::cout << snapshot.time_stamp << " bid_px_00: " << snapshot.bid_px_00 << " ask_px_00: " << snapshot.ask_px_00 << " bid_sz_00: " << snapshot.bid_sz_00 << " ask_sz_00: " << snapshot.ask_sz_00 << std::endl;
    return snapshot;
}



int main() {

    std::ifstream dataFile("first_25000_rows.csv");
    std::string line;
    BestLevelOFI calculator;

    orderbookSnapshot previous, current;
    bool first = true;

    // This will skip the first line, headder. 
    std::getline(dataFile, line);


    for (int i = 0; i < 100; ++i) {
        std::getline(dataFile, line);
        current = parseLineToSnapshot(line);

        if (!first) {
            double ofi = calculator.compute(current, previous);
            std::cout << current.time_stamp << " OFI: " << ofi << std::endl;
        }
        else {
            first = false;
        }

        previous = current;
    }



    return 1;
}