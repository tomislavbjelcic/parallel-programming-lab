import numpy as np
import pyopencl as cl
import time

def boundarypsi(psi, m, n, b, h, w):
    for i in range(b+1, b+w):
        psi[i*(m+2)+0] = i-b

    for i in range(b+w, m+1):
        psi[i*(m+2)+0] = w

    for j in range(1, h+1):
        psi[(m+1)*(m+2)+j] = w

    for j in range(h+1, h+w):
        psi[(m+1)*(m+2)+j] = w-j+h


def main():
    printfreq = 1000 # output frequency
    error = None
    bnorm = None
    tolerance = 0.0 # tolerance for convergence. <=0 means do not check

	# command line arguments
    scalefactor = 64
    numiter = 1000

	# simulation sizes
    bbase=10
    hbase=15
    wbase=5
    mbase=32
    nbase=32

    checkerr = False

    m,n,b,h,w = None, None, None, None, None

    tstart, tstop, ttot, titer = None, None, None, None


    print(f"Scale Factor = {scalefactor}, iterations = {numiter}\n")


	# Calculate b, h & w and m & n
    b = bbase*scalefactor
    h = hbase*scalefactor
    w = wbase*scalefactor
    m = mbase*scalefactor
    n = nbase*scalefactor

    print(f"Running CFD on {m} x {n} grid in serial\n");

    psi = np.zeros((m + 2) * (n + 2), dtype=np.double)
    err_arr = np.zeros(((m + 2) * (n + 2)), dtype=np.double)

    # set the psi boundary conditions
    boundarypsi(psi,m,n,b,h,w)

    bnorm = 0.0
    for i in range(0, m+2):
        for j in range(0, n+2):
            bnorm += psi[i * (m+2) + j] * psi[i * (m+2) + j]

    bnorm = np.sqrt(bnorm)

    # ucitaj program
    src = None
    with open("cfd.cl", "r") as f:
        src = f.read()
    
    # stvori kontekst i program
    ctx = cl.create_some_context(interactive=False)
    queue = cl.CommandQueue(ctx)
    prog = cl.Program(ctx, src).build()

    # pripremi memoriju naprave
    dev_psi = cl.Buffer(ctx, cl.mem_flags.READ_WRITE | cl.mem_flags.COPY_HOST_PTR, hostbuf=psi)
    dev_psitmp = cl.Buffer(ctx, cl.mem_flags.READ_WRITE, size=psi.nbytes)
    dev_err = cl.Buffer(ctx, cl.mem_flags.READ_WRITE, size=err_arr.nbytes)

    # begin iterative Jacobi loop
    print("\nStarting main loop...\n\n");
    tstart = time.time()
    for iter in range(1, numiter+1):
        # calculate psi for next iteration
        evt = prog.jacobistep(queue, psi.shape, None, dev_psi, dev_psitmp, np.int32(m), np.int32(n))
        evt.wait()


        # calculate current error if required
        if checkerr or iter == numiter:
            evt = prog.deltasq(queue, psi.shape, None, dev_psi, dev_psitmp, dev_err, np.int32(m), np.int32(n))
            evt.wait()
            cl.enqueue_copy(queue, err_arr, dev_err)
            error = np.sum(err_arr)

            error = np.sqrt(error)
            error = error/bnorm
        
        if checkerr and error<tolerance:
            print(f"Converged on iteration {iter}")
            break


        # copy back
        evt = prog.copy(queue, psi.shape, None, dev_psitmp, dev_psi, np.int32(m), np.int32(n))
        evt.wait()

        if iter%printfreq == 0:
            if not checkerr:
                print(f"Completed iteration {iter}")
            else:
                print(f"Completed iteration {iter}, error = {error}")
    
    if iter>numiter:
        iter=numiter
    
    tstop = time.time()
    ttot = tstop - tstart
    titer = ttot / iter

    print("\n... finished")
    print(f"After {iter} iterations, the error is {error}")
    print(f"Time for {iter} iterations was {ttot} seconds")
    print(f"Each iteration took {titer} seconds")





        
        



if __name__ == "__main__":
    main()