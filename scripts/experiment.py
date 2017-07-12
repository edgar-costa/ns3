import subprocess
import time
import argparse
import os, errno

tests = ["ECMP_RANDOM_FLOWLET", "ECMP_PER_FLOW", "ECMP_RANDOM"]
errors = [0, 0.001, 0.01, 0.05, 0.1]


parser = argparse.ArgumentParser()
group = parser.add_mutually_exclusive_group()

parser.add_argument('-r','--RunStep',
                    help = "Seed number for the random generators",
                    default = 1)

parser.add_argument('-o','--OutputFolder',
                    help = "Outputfolder name for the whole experiment",
                    default = "test")

args = parser.parse_args()

#make dir
try:
    os.makedirs("outputs/"+args.OutputFolder)
except:
    pass

cmd = 'time ./waf --run "fat-tree --OutputFolder={2} --LinkBandwidth=10Mbps  --Delay=50 --QueueSize=100 --Protocol=TCP --K=4 --Monitor=false --Debug=true --Animation=false --SimulationTime=25 --SizeDistribution=distributions/enterprise_conga_scaled_100.csv  --IntraPodProb=0 --InterPodProb=1 --InterArrivalFlowTime=3200 --ErrorRate={0} --ErrorLink=r_0_a0->r_c0 --EcmpMode={1} --SimulationName={1}_{0} --RunStep={3}" &'


for test in tests:
    for error in errors:
        subprocess.call(cmd.format(error, test, args.OutputFolder, args.RunStep), shell=True)
        time.sleep(10)
