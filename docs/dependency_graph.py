import sys
from os import path
import argparse
from pprint import pprint
import json


class Graph:
    def __init__(self) -> None:
        self.g: dict[str, list[str]] = {}

    def add_dep(self, fr: str, to: str) -> None:
        assert type(fr) is str, fr
        assert type(to) is str, to

        if fr not in self.g:
            self.g[fr] = []
        if to not in self.g:
            self.g[to] = []

        self.g[fr].append(to)

    def get_dct(self) -> dict[str, list[str]]:
        return self.g


def parse_include(row: str, path_prefix: str) -> str:
    row = row.strip()
    to_cut = len('#include')
    assert row[:to_cut] == '#include', row

    row = row[to_cut:].strip()

    # system library
    if row.startswith('<'):
        assert row.endswith('>'), row
        return row

    # project file
    assert row.startswith('"'), row
    assert row.startswith('"'), row

    row = row[1:-1]
    return path.normpath(path.join(path_prefix, row))


def get_include_rows(filename: str) -> list[str]:
    rows = []
    with open(filename, 'r') as f:
        for row in f:
            stripped = row.strip()
            if stripped.startswith('#include'):
                rows.append(stripped)

    return rows


def get_dependencies(filename: str) -> list[str]:
    includes = get_include_rows(filename)
    deps: list[str] = []
    prefix = path.dirname(filename)
    for row in includes:
        deps.append(parse_include(row, prefix))

    return deps


def create_graph(files: list[str]) -> Graph:
    g = Graph()
    for file in files:
        for dep in get_dependencies(file):
            g.add_dep(file, dep)

    return g


def main():
    parser = argparse.ArgumentParser('Create dependecy graph')

    parser.add_argument(
        '-o',
        metavar='JSON',
        type=str,
        required=True,
        help='File to save graph to',
    )

    parser.add_argument(
        'files',
        metavar='PATH',
        type=str,
        nargs='+',
        help='Files to process',
    )

    args = parser.parse_args()

    norm_paths = [path.normpath(file) for file in args.files]
    graph = create_graph(norm_paths)

    with open(args.o, 'w') as f:
        json.dump(graph.get_dct(), f, indent=4)

    print('done')


if __name__ == '__main__':
    main()
