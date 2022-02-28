import argparse
import json


def create_dot(infile: str, outfile: str) -> None:
    with open(infile, 'r') as f:
        dct = json.load(f)

    with open(outfile, 'w') as f:
        f.write('digraph G {\n')

        # create nodes (even empty)
        for node in dct:
            f.write(f'"{node}";\n')

        # create edges
        for node, edges in dct.items():
            for edge in edges:
                f.write(f'"{node}" -> "{edge}";\n')

        f.write('}\n')


def main():
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

    create_dot(args.i, args.o)


if __name__ == '__main__':
    main()
