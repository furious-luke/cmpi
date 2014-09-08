#!/usr/bin/env python

import argparse, random

if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    parser.add_argument('prefix', metavar='R', nargs=1, help='filename prefix')
    parser.add_argument('n_files', metavar='F', type=int, nargs=1, help='number of files')
    parser.add_argument('n_elems', metavar='S', type=int, nargs=1, help='number of elements')
    args = parser.parse_args()

    split = args.n_elems[0]/args.n_files[0]
    mod = args.n_elems[0]%args.n_files[0]
    pos = 0

    for ii in range(args.n_files[0]):
        fn = '%s.%05d'%(args.prefix[0], ii)
        with open(fn, 'w') as fl:
            size = split
            if ii < mod:
                size += 1
            if ii < args.n_files[0] - 1:
                size = min(max(0, size + random.randint(-10, 10)), args.n_elems[0] - pos)
            else:
                size = args.n_elems[0] - pos
            fl.write(str(size) + '\n')
            for jj in range(size):
                fl.write(str(pos) + '\n')
                pos += 1
