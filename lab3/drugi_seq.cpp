#include <iostream>
#include <limits>
#include <cmath>
#include <chrono>

typedef std::numeric_limits< double > dbl;

int main() {

    std::cout.precision(dbl::max_digits10);

    
    const long n = 2e8L;
    std::cout << "N = " << n << '\n';

    auto start = std::chrono::high_resolution_clock::now();

    double sum = 0.0;
    double x;
    for (long i=1; i<=n; i++) {
        x = (i - 0.5) / n;
        sum += 4.0 / (1.0 + x*x);
    }
    double pi = sum / n;

    auto end = std::chrono::high_resolution_clock::now();
    auto duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    double duration_seconds = duration_ns / 1e9;
    
    std::cout << "pi = " << pi << std::endl;
    std::cout << "Odstupanje: " << fabs(pi-M_PI) << std::endl;
    std::cout << "Izvrseno u " << duration_seconds << " sekundi" << std::endl;
    
    

    return 0;
}