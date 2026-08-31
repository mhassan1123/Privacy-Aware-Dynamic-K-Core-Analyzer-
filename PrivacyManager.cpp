#include "PrivacyManager.h"
using namespace std;

PrivacyManager::PrivacyManager() {
    random_device rd;
    generator.seed(rd());
}

double PrivacyManager::laplaceNoise(double scale) {
    if (scale <= 0.0) return 0.0;

    // Generate uniform random variable u in (-0.5, 0.5)
    uniform_real_distribution<double> dist(-0.5, 0.5);
    double u = dist(generator);

    // Inverse CDF transform for Laplace distribution
    // Noise = -scale * sgn(u) * ln(1 - 2|u|)
    double sgn = (u < 0) ? -1.0 : 1.0;
    return -scale * sgn * log(1.0 - 2.0 * abs(u));
}

double PrivacyManager::privatize(double value, double sensitivity, double epsilon) {
    if (epsilon <= 0.0) {
        cerr << "Warning: Invalid epsilon value. Noise injection skipped.\n";
        return value;
    }

    double scale = sensitivity / epsilon;
    double noise = laplaceNoise(scale);
    return value + noise;
}