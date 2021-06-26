"""
scripts/visualize.py

A script to visualize the internal state of solutions.
"""

__author__ = "Kimiyuki Onaka"
__copyright__ = "Copyright (c) 2021 Kimiyuki Onaka"
__credits__ = []  # type: List[str]
__license__ = "MIT"
__version__ = "1.1.0"
__maintainer__ = "Kimiyuki Onaka"
__email__ = "kimiyuki95@gmail.com"
__status__ = "Production"

import argparse
import concurrent.futures
import math
import os
import pathlib
import shutil
import subprocess
import sys
import tempfile
from logging import DEBUG, basicConfig, getLogger
from typing import *
from typing import BinaryIO

logger = getLogger(__name__)

BEGIN_MARKER = b'-----BEGIN-----'
END_MARKER = b'-----END-----'


def gen(*, seed: int) -> None:
    logger.info('running the generator...')
    with open('seeds.txt', 'w') as fh:
        print(seed, file=fh)
    command = [str((pathlib.Path.cwd() / 'tools' / 'target' / 'release' / 'vis').resolve()), 'seeds.txt']
    subprocess.check_call(command)


def parse_result(*, stdout: bytes, stderr: BinaryIO) -> List[bytes]:
    outputs: List[bytes] = []
    lines = stderr.readlines()
    l = 0
    while l < len(lines):
        if lines[l].rstrip() == BEGIN_MARKER:
            r = l + 1
            while r < len(lines) and lines[r].rstrip() != END_MARKER:
                r += 1
            if r == len(lines):
                raise RuntimeError('{} is not found'.format(repr(END_MARKER.decode())))
            outputs.append(b''.join(lines[l + 1:r]))
            l = r + 1
        elif lines[l].rstrip() == END_MARKER:
            raise RuntimeError('unexpected {} found'.format(repr(END_MARKER.decode())))
        else:
            sys.stderr.buffer.write(lines[l])
            l += 1
    outputs.append(stdout)
    return outputs


def vis(*, input_path: pathlib.Path, output_path: pathlib.Path, vis_path: pathlib.Path, index: int) -> int:
    logger.info('running the visualizer for %d-th state...', index)
    with tempfile.TemporaryDirectory() as tempdir_:
        tempdir = pathlib.Path(tempdir_)
        try:
            command = [str((pathlib.Path.cwd() / 'tools' / 'target' / 'release' / 'vis').resolve()), str(input_path.resolve()), str(output_path.resolve())]
            score_bytes = subprocess.check_output(command, cwd=tempdir)
        except subprocess.SubprocessError as e:
            raise RuntimeError('failed for index = %d' % index) from e
        os.rename(tempdir / 'out.svg', vis_path)
    if not score_bytes.startswith(b'Score = '):
        raise RuntimeError(score_bytes.decode())
    return int(score_bytes.split()[2])


def main() -> 'NoReturn':
    parser = argparse.ArgumentParser()
    parser.add_argument('-c', '--command', default='./a.out')
    parser.add_argument('-j', '--jobs', type=int, default=2)
    parser.add_argument('--seed', type=int, default=0)
    args = parser.parse_args()

    basicConfig(level=DEBUG)

    if not pathlib.Path('tools').exists():
        logger.error('tools/ directory is not found')
        sys.exit(1)
    command = ['cargo', 'build', '--manifest-path', str(pathlib.Path('tools', 'Cargo.toml')), '--release']
    subprocess.check_output(command)

    pathlib.Path('in').mkdir(exist_ok=True)
    pathlib.Path('out').mkdir(exist_ok=True)
    pathlib.Path('vis').mkdir(exist_ok=True)

    # gen
    input_path = pathlib.Path('in', '0000.txt')
    gen(seed=args.seed)

    # run
    logger.info('running the command...')
    output_path = pathlib.Path('out', 'err.txt')
    with open(input_path, 'rb') as fh1:
        with open(output_path, 'wb') as fh2:
            proc = subprocess.run(args.command, stdin=fh1, stdout=subprocess.PIPE, stderr=fh2, check=True)
    logger.info('done')
    with open(output_path, 'rb') as fh:
        outputs = parse_result(stdout=proc.stdout, stderr=fh)
    logger.info('%d states found', len(outputs))
    if len(outputs) > 4096:
        logger.info('too many states')
        sys.exit(1)

    # vis
    if pathlib.Path('vis').exists():
        shutil.rmtree(pathlib.Path('vis'))
    logger.info('build the visualizer...')
    subprocess.check_output(command)
    score_futures: List[concurrent.futures.Future] = []
    with concurrent.futures.ProcessPoolExecutor(max_workers=args.jobs) as executor:
        for i, output in enumerate(outputs):
            output_path = pathlib.Path('out', '%04d.txt' % i)
            vis_path = pathlib.Path('vis', '%04d.svg' % i)
            with open(output_path, 'wb') as fh:
                fh.write(output)
            score_futures.append(executor.submit(vis, input_path=input_path, output_path=output_path, vis_path=vis_path, index=i))
    scores = [future.result() for future in score_futures]
    for i, score in enumerate(scores):
        logger.info('index = {}: score = {}'.format(i, score))

    # generate video
    command = ['ffmpeg', '-pattern_type', 'glob', '-i', str(pathlib.Path('vis', '*.svg')), '-framerate', '60', '-vcodec', 'libx264', '-pix_fmt', 'yuv420p', 'vis/out.mp4']
    subprocess.check_call(command)

    if min(scores) <= 0:
        sys.exit(1)
    sys.exit(0)


if __name__ == '__main__':
    main()
