#!/usr/bin/env python3
#---------
# 6 pedestrians created at frame >= 9
# ids are continuous
#
# creation times in sources are 1, 5, 10, 15, 25, 30
#---------
import os
from sys import argv, path
import logging
utestdir = os.path.abspath(os.path.dirname(os.path.dirname(os.path.dirname(path[0]))))
path.append(utestdir)
path.append(os.path.dirname(path[0])) # source helper file
from utils import SUCCESS, FAILURE
from JPSRunTest import JPSRunTestDriver
import numpy as np

# adding 9 frames (fps=8). The distance for the first agent takes exactly 7 seconds.
should_be_7 = np.array([7 + 1.125, 11 + 1.125, 16 + 1.125, 21 + 1.125, 31 + 1.125, 36 + 1.125])
should_be_9 = should_be_7 + 2

def runtest(inifile, trajfile):
    logging.info("===== Method A - Flow-NT ===============")
    data_9_filename = os.path.join('./Output',
                                   'Fundamental_Diagram',
                                   'FlowVelocity',
                                   'Flow_NT_%s_id_9.dat'%trajfile
                                   )
    
    if not os.path.exists(data_9_filename):
        logging.critical("jpsreport did not output results correctly.")
        exit(FAILURE)

    data_9 = np.loadtxt(data_9_filename)
    time_change_9 = data_9[np.nonzero(np.diff(data_9[:, 2]) >0)][:, 1]
    if np.all(np.abs(time_change_9 - should_be_9) < 0.5):
        logging.info("should_be_9 OK.")
    else:
        logging.critical("should_be_9 did not match result. Got {}. Expected {}".format(time_change_9, should_be_9))
        exit(FAILURE)       

    data_7_filename = os.path.join('./Output',
                                   'Fundamental_Diagram',
                                   'FlowVelocity',
                                   'Flow_NT_%s_id_7.dat'%trajfile
                                   )
    if not os.path.exists(data_7_filename):
        logging.error("jpsreport did not output results correctly.")
        exit(FAILURE)

    data_7 = np.loadtxt(data_7_filename)  
    time_change_7 = data_7[np.nonzero(np.diff(data_7[:, 2]) >0)][:, 1]

    if np.all(np.abs(time_change_7 - should_be_7) < 0.5):
        logging.info("should_be_7 OK.")
    else:
        logging.critical("should_be_7 did not match result. Got {}. Expected {}".format(time_change_7, should_be_7))
        exit(FAILURE)       


if __name__ == "__main__":
    test = JPSRunTestDriver(3, argv0=argv[0], testdir=path[0], utestdir=utestdir, jpsreport=argv[1])
    test.run_analysis(trajfile="traj.txt", testfunction=runtest)
    logging.info("%s exits with SUCCESS" % (argv[0]))
    exit(SUCCESS)
