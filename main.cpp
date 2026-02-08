/*
 * PROJECT: HPC Monte Carlo Option Pricer
 * AUTHOR: Elia Di Santo
 * DATE: February 2026
 *
 * FEATURES:
 * - OpenMP Parallelization (High Throughput)
 * - Hull's Historical Volatility Calculation
 * - Standard Error Estimation (95% Confidence Interval)
 * - Dynamic Strike Price (At-The-Money)
 */

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <cmath>
#include <numeric>
#include <random>
#include <chrono>
#include <iomanip> // Formatting
#include <omp.h>   // OpenMP

// Struttura risultati
struct SimulationResult {
    double price;           // Fair Value oggi
    double std_error;       // Incertezza statistica
    double avg_final_price; // Prezzo medio dell'azione a scadenza
};

// --- MODULE 1: Data Reader ---
std::vector<double> read_prices_from_csv(const std::string& filename) {
    std::vector<double> prices;
    std::ifstream file(filename);
    std::string line;

    if (!file.is_open()) {
        std::cerr << "[ERROR] Could not open " << filename << std::endl;
        return {};
    }

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string value_str;
        while (std::getline(ss, value_str, ',')) {} 
        try {
            if (!value_str.empty() && std::isdigit(value_str.back())) { 
                prices.push_back(std::stod(value_str));
            }
        } catch (...) { continue; }
    }
    return prices;
}

// --- MODULE 2: Analytics ---
double calculate_historical_volatility(const std::vector<double>& prices) {
    if (prices.size() < 2) return 0.20;
    std::vector<double> log_returns;
    double sum = 0.0;
    for (size_t i = 1; i < prices.size(); ++i) {
        double u_i = std::log(prices[i] / prices[i - 1]);
        log_returns.push_back(u_i);
        sum += u_i;
    }
    double mean = sum / log_returns.size();
    double sq_sum = 0.0;
    for (double u : log_returns) sq_sum += (u - mean) * (u - mean);
    return std::sqrt(sq_sum / (log_returns.size() - 1)) * std::sqrt(252.0);
}

// --- MODULE 3: HPC Monte Carlo Engine ---
SimulationResult monte_carlo_pricer(double S0, double K, double r, double sigma, double T, int N) {
    double sum_payoffs = 0.0;
    double sum_sq_payoffs = 0.0; 
    double sum_ST = 0.0;         

    double drift = (r - 0.5 * sigma * sigma) * T;
    double vol_sqrt_T = sigma * std::sqrt(T);
    double discount_factor = std::exp(-r * T);

    #pragma omp parallel reduction(+:sum_payoffs, sum_sq_payoffs, sum_ST)
    {
        unsigned int seed = std::chrono::system_clock::now().time_since_epoch().count() + omp_get_thread_num();
        std::mt19937 generator(seed);
        std::normal_distribution<double> distribution(0.0, 1.0);

        #pragma omp for
        for (int i = 0; i < N; ++i) {
            double Z = distribution(generator);
            // Formula geometrica: S_T = S0 * e^(drift + diffusion)
            double S_T = S0 * std::exp(drift + vol_sqrt_T * Z);
            
            // Payoff Call: Max(S_T - K, 0)
            double payoff = std::max(S_T - K, 0.0);

            sum_payoffs += payoff;
            sum_sq_payoffs += payoff * payoff;
            sum_ST += S_T;
        }
    }

    double mean_payoff = sum_payoffs / N;
    double variance = (sum_sq_payoffs / N) - (mean_payoff * mean_payoff);
    double std_error = (std::sqrt(variance) / std::sqrt(N)) * discount_factor;

    return {mean_payoff * discount_factor, std_error, sum_ST / N};
}

int main(int argc, char* argv[]) {
    // Configurazione Input
    std::string csv_file = (argc > 1) ? argv[1] : "market_data.csv";
    
    // Parametri Default
    double S0 = 100.0;   // Prezzo Iniziale (Spot)
    double K = 100.0;    // Prezzo Strike (Target)
    double r = 0.05;     // Tasso Risk-Free (5%)
    double T = 1.0;      // Scadenza (1 Anno)
    int N = 1000000000;    // 1000 Milioni di simulazioni
    double volatility = 0.2;

    std::cout << "============================================" << std::endl;
    std::cout << "   HPC MONTE CARLO PRICER (C++ / OpenMP)    " << std::endl;
    std::cout << "============================================" << std::endl;
    
    std::cout << "[SYSTEM]   CPU Cores: " << omp_get_num_procs() << std::endl;
    std::cout << "[SYSTEM]   Threads:   " << omp_get_max_threads() << std::endl;

    // Caricamento Dati
    std::vector<double> history = read_prices_from_csv(csv_file);
    if (!history.empty()) {
        volatility = calculate_historical_volatility(history);
        S0 = history.back();
        K = S0; // <--- MODIFICA: Imposta Strike uguale al prezzo Spot (At-The-Money)
        
        std::cout << "[DATA]     Source: " << csv_file << " (" << history.size() << " points)" << std::endl;
        std::cout << "[DATA]     Historical Volatility: " << std::fixed << std::setprecision(2) << volatility * 100 << "%" << std::endl;
    } else {
        std::cout << "[WARNING]  File not found. Using dummy params." << std::endl;
    }

    std::cout << "[RUN]      Simulating " << std::scientific << std::setprecision(2) << (double)N << " paths..." << std::endl;
    
    auto start = std::chrono::high_resolution_clock::now();
    SimulationResult res = monte_carlo_pricer(S0, K, r, volatility, T, N);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    // --- REPORT FINALE ---
    std::cout << "--------------------------------------------" << std::endl;
    std::cout << std::fixed << std::setprecision(4);
    
    std::cout << "Simulation Parameters:" << std::endl;
    std::cout << "  > Asset Start Price (S0): " << S0 << " EUR" << std::endl;
    std::cout << "  > Option Strike Price (K):" << K << " EUR" << std::endl;
    std::cout << "  > Time to Maturity (T):   " << T << " Years" << std::endl;
    std::cout << "  > Risk-Free Rate (r):     " << r * 100 << " %" << std::endl;
    std::cout << "--------------------------------------------" << std::endl;
    
    std::cout << "Asset Projection (Drift Check):" << std::endl;
    std::cout << "  > Avg Final Price (ST):   " << res.avg_final_price << " EUR" << std::endl;
    std::cout << "--------------------------------------------" << std::endl;

    std::cout << "Option Valuation (95% Confidence):" << std::endl;
    std::cout << "  > FAIR VALUE:             " << res.price << " EUR" << std::endl;
    std::cout << "  > Standard Error:         " << res.std_error << std::endl;
    std::cout << "  > Conf. Interval:         [" 
              << res.price - 1.96 * res.std_error << ", " 
              << res.price + 1.96 * res.std_error << "]" << std::endl;
    
    std::cout << "--------------------------------------------" << std::endl;
    std::cout << "Performance Metrics:" << std::endl;
    std::cout << "  > Time:                   " << std::setprecision(5) << elapsed.count() << " sec" << std::endl;
    std::cout << "  > Throughput:             " << std::setprecision(2) << (N / elapsed.count()) / 1e6 << " M sims/sec" << std::endl;
    std::cout << "============================================" << std::endl;

    return 0;
}