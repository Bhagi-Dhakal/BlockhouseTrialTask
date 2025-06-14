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

/* Order Book Snapshots  */
struct orderbookSnapshot {
    std::string time_stamp;
    std::vector<double> bid_px;
    std::vector<double> ask_px;
    std::vector<int> bid_sz;
    std::vector<int> ask_sz;
};

// struct multiLevelOrderbookSnapshot {
//     std::string time_stamp;
//     std::vector<double> bid_px;
//     std::vector<double> ask_px;
//     std::vector<int> bid_sz;
//     std::vector<int> ask_sz;
// };

/* Bid and Ask logic */
std::vector <int> bidLogic(const orderbookSnapshot& current, const orderbookSnapshot& previous, int level) {
    std::vector <int> bid;
    for (int i = 0; i < level; ++i) {
        if (current.bid_px[i] > previous.bid_px[i]) {
            bid.push_back(current.bid_sz[i]);
        }
        else if (current.bid_px[0] == previous.bid_px[i]) {
            bid.push_back(current.bid_sz[i] - previous.bid_sz[i]);
        }
        else {
            // current.bid_px[i] < previous.bid_px[i]
            bid.push_back(-current.bid_sz[i]);
        }
    }
    return bid;
}

std::vector <int> askLogic(const orderbookSnapshot& current, const orderbookSnapshot& previous, int level) {
    std::vector <int> ask;
    for (int i = 0; i < level; ++i) {
        if (current.ask_px[i] > previous.ask_px[i]) {
            ask.push_back(-current.ask_sz[i]);
        }
        else if (current.ask_px[i] == previous.ask_px[i]) {
            ask.push_back(current.ask_sz[i] - previous.ask_sz[i]);
        }
        else {
            // current.ask_px[i] < previous.ask_px[i]
            ask.push_back(current.ask_sz[i]);
        }
    }
    return ask;
}



/* Construction of Best-Level OFI */
class BestLevelOFI {
public:
    int compute(const orderbookSnapshot& current, const orderbookSnapshot& previous) {
        int bid_size = bidLogic(current, previous, 1)[0];
        int ask_size = askLogic(current, previous, 1)[0];

        return bid_size - ask_size;
    }
};

class DeeperLevelOFI {
public:
    std::vector<double> compute(const orderbookSnapshot current, const orderbookSnapshot& previous, int level) {

        std::vector<double> multiOFI;
        std::vector<int> bid_size = bidLogic(current, previous, level);
        std::vector<int> ask_size = askLogic(current, previous, level);

        for (int i = 0; i < level; ++i) {
            multiOFI.push_back(bid_size[i] - ask_size[i]);
        }

        return multiOFI;
    }


    // need to compute average depth

    // need to normalize the OFI 

    // need to wrap everyting into nice function


};

/* File Line Parser to orderbookSnapshot */
orderbookSnapshot parseLineToSnapshot(const std::string& line, int level) {
    std::stringstream ss(line);
    std::string token;
    std::vector<std::string> fields;

    // Reads each, value seperated by , in the line
    while (getline(ss, token, ',')) {
        fields.push_back(token);
    }

    orderbookSnapshot multi_snapshot;
    multi_snapshot.time_stamp = fields[0];

    for (int i = 0; i < level; ++i) {
        multi_snapshot.bid_px.push_back(std::stod(fields[13 + i * 6]));
        multi_snapshot.ask_px.push_back(std::stod(fields[14 + i * 6]));
        multi_snapshot.bid_sz.push_back(std::stod(fields[15 + i * 6]));
        multi_snapshot.ask_sz.push_back(std::stod(fields[16 + i * 6]));
    }

    //std::cout << snapshot.time_stamp << " bid_px[i]: " << snapshot.bid_px[i] << " ask_px[i]: " << snapshot.ask_px[i] << " bid_sz[i]: " << snapshot.bid_sz[i] << " ask_sz[i]: " << snapshot.ask_sz[i] << std::endl;
    return multi_snapshot;
}


int main() {
    int level = 10;
    std::ifstream dataFile("first_25000_rows.csv");
    std::string line;
    DeeperLevelOFI calculator;

    orderbookSnapshot previous, current;
    bool first = true;

    // This will skip the first line, headder. 
    std::getline(dataFile, line);


    for (int i = 0; i < 100; ++i) {
        std::getline(dataFile, line);
        current = parseLineToSnapshot(line, level);

        if (!first) {
            std::vector<double> ofi = calculator.compute(current, previous, level);
            std::cout << current.time_stamp;
            for (int j = 0; j < level; ++j) {
                std::cout << i << ":  " << " OFI: " << ofi[j] << std::endl;
            }
        }
        else {
            first = false;
        }

        previous = current;
    }



    return 1;
}