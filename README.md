# HPC Monte Carlo Option Pricer

A high-performance C++ engine designed to price **European Call Options** using **Monte Carlo simulations**. 
This project leverages **OpenMP** for parallel computing, enabling the simulation of millions of market scenarios in milliseconds.

It includes a module for **Historical Volatility Analysis** based on John C. Hull's methodology (Log-Returns Standard Deviation), allowing the model to adapt to real-world market data input.

---

## Key Features

* **Parallel Computing:** Uses **OpenMP** to distribute simulations across all available CPU cores, achieving linear scalability.
* **Mathematical Robustness:** Implements **Geometric Brownian Motion (GBM)** for asset path generation.
* **Statistical Precision:** Calculates **Standard Error** and **95% Confidence Intervals** for rigorous risk assessment.
* **Dynamic Data Analysis:** Parses CSV market data to automatically estimate volatility ($\sigma$) and set the Spot Price ($S_0$).
* **Professional Build System:** Includes a generic `Makefile` for automated compilation and benchmarking.

## Performance
On a standard consumer workstation (8-core / 16-thread CPU), the engine achieves a throughput of **~150 Million simulations per second**.

| Metric | Value |
| :--- | :--- |
| **Simulations** | 10,000,000 |
| **Execution Time** | ~0.06 seconds |
| **Parallel Efficiency** | > 90% scaling on multi-core |

---

## Build & Run

### Prerequisites
* **g++** (GCC) compiler with OpenMP support.
* **Make** (standard build tool).

### Quick Start (Linux/macOS)

1.  **Compile the project:**
    ```bash
    make
    ```

2.  **Run with default settings (auto-detect cores):**
    ```bash
    make run
    ```

3.  **Run with custom thread count (e.g., 8 threads):**
    ```bash
    make run THREADS=8
    ```

4.  **Run with custom data file:**
    ```bash
    make run DATA=my_stock_data.csv
    ```

---

## Mathematical Model

The asset price path is simulated using the exact solution to the **Geometric Brownian Motion** SDE:

$$ S_T = S_0 \cdot \exp\left( (r - \frac{1}{2}\sigma^2)T + \sigma\sqrt{T} \cdot Z \right) $$

Where:
* $S_T$: Asset price at maturity
* $r$: Risk-free interest rate
* $\sigma$: Volatility (derived from historical data)
* $Z$: Random variable $\sim N(0,1)$ (generated via Mersenne Twister `std::mt19937`)

The **Fair Value** is calculated as the discounted mean of the payoffs:

$$ V = e^{-rT} \cdot \mathbb{E}[\max(S_T - K, 0)] $$

---

## Sample Output

```text
============================================
   HPC MONTE CARLO PRICER (C++ / OpenMP)    
============================================
[SYSTEM]   CPU Cores: 16
[SYSTEM]   Threads:   8
[DATA]     Source: market_data.csv (10 points)
[DATA]     Historical Volatility: 25.72%
[RUN]      Simulating 1.00e+07 paths...
--------------------------------------------
Asset Simulation:
  > Start Price (S0):     106.2000 EUR
  > Avg Final Price (ST): 111.6394 EUR
--------------------------------------------
Option Valuation (95% Confidence):
  > Fair Value:           16.7540 EUR
  > Standard Error:       0.0070
  > Conf. Interval:       [16.7404, 16.7677]
--------------------------------------------
Performance Metrics:
  > Time:                 0.06685 sec
  > Throughput:           149.60 M sims/sec
============================================
