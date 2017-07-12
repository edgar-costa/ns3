import numpy as np
import matplotlib.pyplot as plt
import sys
from fct import Fct

if __name__ == "__main__":
    
    tests = ["ECMP_RANDOM_FLOWLET", "ECMP_PER_FLOW", "ECMP_RANDOM"]
    errors = [0, 0.001, 0.01, 0.05, 0.1]

    test_seed = sys.argv[1]

    root_path = "/home/edgar/ns-allinone-3.26/ns-3.26/outputs/"
    root_name = "fat-tree-{0}_{1}_{2}.fct"


    plt.figure(1)
    color = ["r--", "g--", "b--", "y--", "k--"]
    i =1
    for test in tests:
        
        plt.subplot(310+i)
        
        color_i = 0
        for error in errors:
            fct_reader = Fct(root_path+root_name.format(test, error, test_seed))
            fct = fct_reader.get_attribute("fct")
            
            fct_sorted = sorted(fct)
    
            #log scale

            #y axis step
            y = 1. * np.arange(len(fct))
            y = [x/(len(fct)-1) for x in y]

            ##y = comulative_prob[:]
            
            plt.plot(fct_sorted, y, color[color_i])
            plt.xscale('log')
            color_i +=1

        i +=1

        
    plt.savefig("run_"+test_seed)
    plt.show()
    
    
    
