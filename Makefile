# --- Makefile for HPC Option Pricer ---

# Configurazione (Modificabili da riga di comando)
THREADS ?= 8
DATA    ?= market_data.csv

# Compilatore e Flag
CXX      = g++
CXXFLAGS = -O3 -fopenmp -Wall -std=c++11
TARGET   = pricer
SRC      = main.cpp

# --- Regole ---

all: $(TARGET)

$(TARGET): $(SRC)
	@echo "[BUILD] Compiling $(TARGET)..."
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)
	@echo "[SUCCESS] Ready."

clean:
	@echo "[CLEAN] Removing executable..."
	rm -f $(TARGET)

# La regola RUN passa il nome del file come argomento al programma C++
run: $(TARGET)
	@echo "[RUN] Executing on $(THREADS) threads using data: $(DATA)..."
	OMP_NUM_THREADS=$(THREADS) ./$(TARGET) $(DATA)