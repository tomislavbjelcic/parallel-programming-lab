

__kernel void calc_pi(__global double *out, const long n) {
    int global_id = get_global_id(0);
    int global_size = get_global_size(0);

    double sum = 0.0;
    double x;
    for (long i = global_id + 1; i <= n; i += global_size) {
        x = (i - 0.5) / n;
        sum += 4.0 / (1.0 + x*x);
    }
    out[global_id] = sum / n;
    
}