import sys
import os
from os import path
import argparse
import json
from graph import Graph


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


def get_include_rows(filename: str, src: str) -> list[str]:
    rows = []
    with open(path.join(src, filename), 'r') as f:
        for row in f:
            stripped = row.strip()
            if stripped.startswith('#include'):
                rows.append(stripped)

    return rows


def get_dependencies(filename: str, src: str) -> list[str]:
    includes = get_include_rows(filename, src)
    deps: list[str] = []
    prefix = path.dirname(filename)
    for row in includes:
        deps.append(parse_include(row, prefix))

    return deps


def create_graph(files: list[str], src: str) -> Graph:
    g = Graph()
    for file in files:
        for dep in get_dependencies(file, src):
            g.add_dep(file, dep)

    return g


def all_files(prefix: str, src: str) -> list[str]:
    out = []
    for entry in os.scandir(src):
        if 'tests' in entry.name:
            continue

        if entry.is_file():
            if path.splitext(entry.name)[-1] in {'.cpp', '.hpp', '.cxx', '.h', '.cc'}:
                out.append(path.join(prefix, entry.name))

        elif entry.is_dir():
            out += all_files(
                path.join(prefix, entry.name),
                path.join(src, entry.name)
            )

    return out


def main() -> None:
    parser = argparse.ArgumentParser('Create dependecy graph')

    parser.add_argument(
        '-o',
        metavar='JSON',
        type=str,
        required=True,
        help='File to save graph to',
    )

    parser.add_argument(
        '-s',
        metavar='PATH',
        type=str,
        required=True,
        help='Path to root of source directory',
    )

    args = parser.parse_args()

    files = all_files('', args.s)
    print(f'Found {len(files)} files to parse.')
    norm_paths = [path.normpath(file) for file in files]
    graph = create_graph(norm_paths, args.s)

    with open(args.o, 'w') as f:
        json.dump(graph.get_dct(), f, indent=4)

    print('done')


if __name__ == '__main__':
    main()
