import subprocess
import time

tests = ["ECMP_RANDOM_FLOWLET", "ECMP_PER_FLOW", "ECMP_RANDOM"]
errors = [0, 0.001, 0.01, 0.05, 0.1]


cmd = 'time ./waf --run "fat-tree --LinkBandwidth=10Mbps  --Delay=50 --QueueSize=100 --Protocol=TCP --K=4 --Monitor=false --Debug=true --Animation=false --SimulationTime=30 --SizeDistribution=distributions/enterprise_conga_scaled_100.csv  --IntraPodProb=0 --InterPodProb=1 --InterArrivalFlowTime=1600 --ErrorRate={0} --ErrorLink=r_0_a0->r_c0 --EcmpMode={1} --ExperimentName={1}_{0} --RunStep=1" &'


for test in tests:
    for error in errors:
        subprocess.call(cmd.format(error, test), shell=True)
        time.sleep(8)
