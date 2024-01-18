import numpy as np
import pyopencl as cl
import time

def main():

    # stvaranje konteksta i dohvacanje informacija o napravi
    ctx = cl.create_some_context(interactive=False)
    dev = ctx.get_info(cl.context_info.DEVICES)[0]
    L_multiple = dev.get_info(cl.device_info.PREFERRED_WORK_GROUP_SIZE_MULTIPLE)
    
    # podesi parametre
    N = 10**10 #int(input("N = "))
    if N < 1:
        return
    G = 2**12
    L = L_multiple * 1
    print(f"N = {N}\nG = {G}\nL = {L}")
    

    # ucitaj program
    src = None
    with open("calc_pi.cl", "r") as f:
        src = f.read()
    

    # pripremi podatke
    # nema ih...
    

    # stvori red
    queue = cl.CommandQueue(ctx)


    # pripremi memoriju naprave
    dev_results = cl.Buffer(context=ctx, flags=cl.mem_flags.READ_WRITE, size=G * np.dtype(np.double).itemsize)


    # prevedi i izvrsi program
    prog = cl.Program(ctx, src).build()
    start_time = time.time()
    evt = prog.calc_pi(queue, (G, ), (L, ), dev_results, np.int64(N))
    evt.wait()
    duration = time.time() - start_time

    # dohvati rezultate
    results = np.empty(G, dtype=np.double)
    cl.enqueue_copy(queue, results, dev_results)
    res = np.sum(results)
    print(f"pi = {res}")
    print(f"Odstupanje od numpy.pi: {np.absolute(res-np.pi)}")
    print(f"Dovrseno u {duration} sekundi.")



if __name__ == '__main__':
    main()