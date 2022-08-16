import argparse
import json
from os import path
from graph import Graph

COLORS = {
    '.cpp': 'cyan',
    '.cxx': 'greenyellow',
    '.h': 'lightcoral',
    '.hpp': 'indianred1',
}


def create_dot(g: Graph, outfile: str) -> None:
    dct = g.get_dct()

    with open(outfile, 'w') as f:
        f.write('digraph G {\n')

        # create nodes (even empty)
        for node in dct:
            color = COLORS.get(path.splitext(node)[-1], 'white')
            f.write(
                f'"{node}" [label="{node}", fillcolor={color}, style=filled, shape=oval];\n')

        # create edges
        for node, edges in dct.items():
            for edge in edges:
                f.write(f'"{node}" -> "{edge}";\n')

        f.write('}\n')


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '-i',
        metavar='JSON',
        required=True,
        type=str,
        help='Path to json file containing graph',
    )

    parser.add_argument(
        '-o',
        metavar='DOT',
        required=True,
        type=str,
        help='Path to dot output file'
    )

    args = parser.parse_args()
    with open(args, 'r') as f:
        g = Graph(json.load(f))

    create_dot(g, args.o)


if __name__ == '__main__':
    main()
