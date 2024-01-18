__kernel void jacobistep(__global double* psi, __global double* psitmp, const int m, const int n) {
    int global_id = get_global_id(0);
    int c = m + 2;
    if (global_id >= m + 3 && global_id <= m * m + 2 * m + n && (global_id % (m + 2) != m + 1 && global_id % (m + 2) != 0)) {
        psitmp[global_id] = 0.25 * (psi[global_id - c] + psi[global_id + c] + psi[global_id - 1] + psi[global_id + 1]);
    }
};


__kernel void copy(__global const double* psitmp, __global double* psi, const int m, const int n) {
    int global_id = get_global_id(0);
    if (global_id >= m + 3 && global_id <= m * m + 2 * m + n && (global_id % (m + 2) != m + 1 && global_id % (m + 2) != 0)) {
        psi[global_id] = psitmp[global_id];
    }
}


__kernel void deltasq(__global const double* psi, __global const double* psitmp, __global double* sum, const int m, const int n) {
    int global_id = get_global_id(0);
    if (global_id >= m + 3 && global_id <= m * m + 2 * m + n && (global_id % (m + 2) != m + 1 && global_id % (m + 2) != 0)) {
        double temp = psitmp[global_id] - psi[global_id];
        sum[global_id] += (temp * temp);
    }
}
