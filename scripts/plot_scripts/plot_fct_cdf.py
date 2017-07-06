import numpy as np
import matplotlib.pyplot as plt
import sys


if __name__ == "__main__":

    with open(sys.argv[1], "r") as f:
        fct = f.readlines()

    #get flow completion time and convert
    #convert to float
    lines = [x.strip().split("  ") for x in fct]

    print lines

    fct = [float(x[0]) for x in lines]
    ##comulative_prob = [float(x[1]) for x in lines]

    fct_sorted = sorted(fct)

    #log scale

    #y axis step
    y = 1. * np.arange(len(fct))
    y = [x/(len(fct)-1) for x in y]

    ##y = comulative_prob[:]

    plt.plot(fct_sorted, y)
    plt.xscale('log')
    plt.show()
    
    
    
