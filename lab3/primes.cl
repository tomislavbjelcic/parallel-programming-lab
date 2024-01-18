
int is_prime(int n) {
    if (n < 2)
        return 0;
    int to = floor(sqrt((double)n));
    for (int i=2; i<=to; i++) {
        if (n%i == 0)
            return 0;
    }
    return 1;
}


__kernel void count_primes(__global const int *arr, __global int *out, const int n) {
    int global_id = get_global_id(0);
    // int local_id = get_local_id(0);
    const int mem_offset = n*global_id;
    for (int i=0; i<n; i++) {
        if (is_prime(arr[mem_offset + i])) {
            // (*out)++;
            atomic_inc(out);
        }
    }
}