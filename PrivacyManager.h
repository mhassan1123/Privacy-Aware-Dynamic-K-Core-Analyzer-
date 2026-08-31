#ifndef PRIVACYMANAGER_H
#define PRIVACYMANAGER_H

#include <random>
#include <cmath>
#include <iostream>
#include <vector>

using namespace std;

class PrivacyManager {
private:
    mt19937 generator;

public:
    // Constructor initializes the random number engine with a hardware seed
    PrivacyManager();

    // Generates a random sample from Laplace distribution L(0, scale)
    double laplaceNoise(double scale);

    // Applies Laplace noise to privatize a target value
    double privatize(double value, double sensitivity, double epsilon);
};

#endif // PRIVACYMANAGER_H