#!/usr/bin/env python3
# coding: utf-8

import numpy as np
import matplotlib.pyplot as plt
from argparse import ArgumentParser
import os

parser = ArgumentParser(usage="Compare 2 files as outputted by sfizz_plot_lfo")
parser.add_argument("file", help="The file to test")
parser.add_argument("reference", help="The reference file")
args = parser.parse_args()

assert os.path.exists(args.file), "The file to test does not exist"
assert os.path.exists(args.reference), "The reference file does not exist"

reference_data = np.loadtxt(args.reference)
data = np.loadtxt(args.file)

assert reference_data.shape == data.shape, "The shapes of the data and reference are different"

n_samples, n_lfos = reference_data.shape
n_rows = n_lfos // 4
fig, ax = plt.subplots(n_lfos // 4, 4, figsize=(20, 10))
for i in range(n_rows):
    for j in range(4):
        lfo_index = 1 + i * 4 + j
        ax[i, j].plot(reference_data[:, 0], reference_data[:, lfo_index])
        ax[i, j].plot(data[:, 0], data[:, lfo_index])
        ax[i, j].set_title("LFO {}".format(lfo_index))
        ax[i, j].grid()
plt.show()
