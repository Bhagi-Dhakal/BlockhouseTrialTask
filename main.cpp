/*
    Author: Bhagi Dhakal
    BlockHouse Trial Task 1: Order Flow Imbalances (OFI)
    Description: Construction of the following OFI features, Best-Level OFI,
        Multi-Level OFI, Integrated OFI and Cross-Asset OFI. All this is from:
        Cross-impact of order flow imbalance in equity markets.

    File Name: main.cpp
    Compile: g++ -std=c++20 -I/Library/eigen-3.4.0 main.cpp -o main
*/

/* Includes */
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <Eigen/Dense>

/* Order Book Snapshots  */
struct orderbookSnapshot {
    std::string time_stamp;
    std::vector<double> bid_px;
    std::vector<double> ask_px;
    std::vector<int> bid_sz;
    std::vector<int> ask_sz;
};

/* Bid and Ask logic (Section 2.1 Data)*/
std::vector <int> bidLogic(const orderbookSnapshot& current, const orderbookSnapshot& previous, int level) {
    std::vector <int> bid;
    for (int i = 0; i < level; ++i) {
        if (current.bid_px[i] > previous.bid_px[i]) {
            bid.push_back(current.bid_sz[i]);
        }
        else if (current.bid_px[i] == previous.bid_px[i]) {
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

/*** Compute Raw OFI (Section 2.1.1. Best-level OFI) ***/
std::vector<double> computeRawOFI(const orderbookSnapshot& current, const orderbookSnapshot& previous, int level) {

    std::vector<double> multiOFI;
    std::vector<int> bid_size = bidLogic(current, previous, level);
    std::vector<int> ask_size = askLogic(current, previous, level);

    for (int i = 0; i < level; ++i) {
        multiOFI.push_back(bid_size[i] - ask_size[i]);
    }

    return multiOFI;
}

/*** Log Return (Section 2.1.4. Logarithmic returns )***/
double computeLogReturn(const orderbookSnapshot& current, const orderbookSnapshot& previous) {
    double p_t = 0.5 * (current.bid_px[0] + current.ask_px[0]);
    double p_t_h = 0.5 * (previous.bid_px[0] + previous.ask_px[0]);
    return std::log(p_t / p_t_h);
}


/***  Construction of Best-Level OFI  (Section 2.1.1. Best-level OFI) ***/
class BestLevelOFI {
public:
    int compute(const orderbookSnapshot& current, const orderbookSnapshot& previous) {
        return computeRawOFI(current, previous, 1)[0];
    }
};

/*** Construction of Deeper-Level OFI (Section 2.1.2. Deeper-level OFI) ***/
class DeeperLevelOFI {
private:
    double computeAverageDepth(const orderbookSnapshot& current, const orderbookSnapshot& previous, int level) {
        double average_depth = 0.0;

        for (int i = 0; i < level; ++i) {
            double depth = current.bid_sz[i] + current.ask_sz[i] + previous.bid_sz[i] + previous.ask_sz[i];
            average_depth += depth / 4;
        }

        return average_depth / level;
    }

    std::vector<double> nomalizeRawOFI(std::vector<double>& rawOFI, double average_depth, int level) {
        std::vector<double> OFI;

        for (int i = 0; i < level; ++i) {
            OFI.push_back(rawOFI[i] / average_depth);
        }
        return OFI;
    }

public:
    std::vector<double> compute(const orderbookSnapshot& current, const orderbookSnapshot& previous, int level) {
        std::vector<double> raw_OFI = computeRawOFI(current, previous, level);
        double average_depth = computeAverageDepth(current, previous, level);
        return nomalizeRawOFI(raw_OFI, average_depth, level);
    }
};

/*** Construction of Intergrated OFI (Section 2.1.3. Integrated OFI ) ***/
class IntegratedOFI {
private:
    // FPC 
    Eigen::VectorXd w1;
    bool w1_computed = false;

    void computeFPCHistoricalOFI(const std::vector<std::vector<double>>& historicalOFI, int level) {
        int timeStamps = historicalOFI.size();

        Eigen::MatrixXd X(timeStamps, level);
        for (int t = 0; t < timeStamps; ++t) {
            for (int l = 0; l < level; ++l) {
                X(t, l) = historicalOFI[t][l];
            }
        }

        Eigen::VectorXd mean = X.colwise().mean();
        for (int i = 0; i < X.rows(); ++i) {
            X.row(i) -= mean.transpose();
        }

        // Computing Covariance amatrix 
        Eigen::MatrixXd cov = (X.transpose() * X) / (timeStamps - 1);

        Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> solver(cov);

        // one with the largest eigen values (solves gets it in assending order)
        w1 = solver.eigenvectors().col(level - 1);

        // Normalize the W1 by the l1 norm
        w1 = w1 / w1.lpNorm<1>();

        w1_computed = true;
    }
public:
    void train(const std::vector<std::vector<double>>& historicalOFI, int level) {
        computeFPCHistoricalOFI(historicalOFI, level);
    }

    double compute(const orderbookSnapshot& current, const orderbookSnapshot& previous, int level) {
        if (!w1_computed) {
            throw std::runtime_error("Need to train first! ");
        }

        std::vector<double> rawOFI = computeRawOFI(current, previous, level);

        Eigen::VectorXd ofi_vector(level);
        for (int i = 0; i < level; ++i) {
            ofi_vector(i) = rawOFI[i];
        }

        return w1.dot(ofi_vector);
    }
};

/*** Construction of Cross Impact BestLevel OFI (Section 3.1.1. Price impact of best-level OFIs) ***/
class CrossImpactBestLevelOFI {
public:
    std::pair<double, double> runOLSRegression(const std::vector<double>& x, const std::vector<double>& y) {
        int n = x.size();
        if (n != y.size() || n == 0) return { 0.0, 0.0 };

        Eigen::MatrixXd X(n, 2);
        for (int i = 0; i < n; ++i) {
            X(i, 0) = 1.0;
            X(i, 1) = x[i];
        }

        Eigen::VectorXd Y(n);
        for (int i = 0; i < n; ++i) {
            Y(i) = y[i];
        }

        Eigen::Vector2d beta = (X.transpose() * X).ldlt().solve(X.transpose() * Y);
        return { beta(0), beta(1) };
    }
};


/*** ile Line Parser to orderbookSnapshot ***/
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
        multi_snapshot.bid_sz.push_back(std::stoi(fields[15 + i * 6]));
        multi_snapshot.ask_sz.push_back(std::stoi(fields[16 + i * 6]));
    }
    return multi_snapshot;
}


/*** Test Functions  ***/

/* Test Best Level OFI */
void testBestLevelOFI(std::ifstream& datafile, int level) {
    std::cout << " TEST: BEST LEVEL OFI " << std::endl;

    std::string line;
    orderbookSnapshot previous, current;
    bool first = true;
    datafile.seekg(0, std::ios::beg);

    BestLevelOFI calculator;

    // This will skip the first line, headder. 
    std::getline(datafile, line);

    for (int i = 0; i < 100; ++i) {
        std::getline(datafile, line);
        current = parseLineToSnapshot(line, level);

        if (!first) {
            double bestLevelOFI = calculator.compute(current, previous);
            std::cout << i + 1 << "| Timestamp: " << current.time_stamp << " | Best Level OFI: " << bestLevelOFI << std::endl;
        }
        else {
            first = false;
        }
        previous = current;
    }
    std::cout << std::endl;

}

/* Test Deeper Level OFI */
void testDeeperLevelOFI(std::ifstream& datafile, int level) {
    std::cout << " TEST: DEEPER LEVEL OFI " << std::endl;

    std::string line;
    orderbookSnapshot previous, current;
    bool first = true;
    datafile.seekg(0, std::ios::beg);

    DeeperLevelOFI calculator;

    // This will skip the first line, headder. 
    std::getline(datafile, line);

    // looking at the first 5 rows 
    for (int i = 0; i < 6; ++i) {
        std::getline(datafile, line);
        current = parseLineToSnapshot(line, level);

        if (!first) {
            std::vector<double> ofi = calculator.compute(current, previous, level);
            std::cout << i << "| Timestamp: " << current.time_stamp << std::endl;
            for (int j = 0; j < level; ++j) {
                std::cout << "  " << j + 1 << " | Deeper Level OFI: " << ofi[j] << std::endl;
            }
        }
        else {
            first = false;
        }
        previous = current;

        std::cout << std::endl;
    }
    std::cout << std::endl;
}

/* Test Integrated OFI */
void testIntegratedOFI(std::ifstream& datafile, int level) {
    std::cout << " TEST: INTEGRATED OFI " << std::endl;

    std::string line;
    orderbookSnapshot previous, current;
    bool first = true;
    datafile.seekg(0, std::ios::beg);

    IntegratedOFI integratedOFICalculator;
    std::vector<std::vector<double>> OFITrainData;

    // This will skip the first line, headder. 
    std::getline(datafile, line);

    // collect the historical data
    for (int i = 0; i < 1000; ++i) {
        std::getline(datafile, line);
        current = parseLineToSnapshot(line, level);

        if (!first) {
            std::vector<double> rawOFI = computeRawOFI(current, previous, level);
            OFITrainData.push_back(rawOFI);
        }
        else {
            first = false;
        }

        previous = current;
    }

    // Train to get w1 
    integratedOFICalculator.train(OFITrainData, level);
    std::cout << "Training completed using " << OFITrainData.size() << " snapshots.\n";

    // Use the trained model on the next 100 OFIs
    int used = 0;
    for (int i = 0; i < 100; ++i)
    {
        std::getline(datafile, line);
        current = parseLineToSnapshot(line, level);
        double integratedOFI = integratedOFICalculator.compute(current, previous, level);

        std::cout << i + 1 << "| Timestamp: " << current.time_stamp << " | Integrated OFI: " << integratedOFI << std::endl;

        previous = current;
    }

    std::cout << std::endl;

}

/* Test Cross Impact Best Level OFI */
void testCrossImpactBestLevelOFI(std::ifstream& datafile, int level) {
    std::cout << " TEST: CROSS IMPACT BEST LEVEL OFI " << std::endl;

    std::string line;
    orderbookSnapshot previous, current;
    bool first = true;
    datafile.seekg(0, std::ios::beg);

    // skip the header 
    std::getline(datafile, line);

    CrossImpactBestLevelOFI CIBLCalculator;


    std::vector<double> OFITrainData;
    std::vector<double> logReturns;

    // Training on the first 2000 rows
    for (int i = 0; i < 2000; ++i) {
        std::getline(datafile, line);

        current = parseLineToSnapshot(line, 1);

        if (!first) {
            double ofi = computeRawOFI(current, previous, 1)[0];
            double logValue = computeLogReturn(current, previous);

            OFITrainData.push_back(ofi);
            logReturns.push_back(logValue);
        }
        else {
            first = false;
        }

        previous = current;
    }

    std::pair<double, double> model = CIBLCalculator.runOLSRegression(OFITrainData, logReturns);
    std::cout << "Alpha: " << model.first << " Beta: " << model.second << std::endl << std::endl;

}


/***  Main Function ***/
int main() {
    std::ifstream dataFile("first_25000_rows.csv");

    // All the tests
    testBestLevelOFI(dataFile, 1);
    testDeeperLevelOFI(dataFile, 10);
    testIntegratedOFI(dataFile, 10);
    testCrossImpactBestLevelOFI(dataFile, 10);

    return 1;
}