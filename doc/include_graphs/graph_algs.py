import argparse
import json
from typing import Optional, Any, Iterable
from graph import Graph
from graph_to_dot import create_dot
import os
from collections import deque


class utils:
    class dfs:
        NOT_SEEN = 0
        ENTERED = 1
        LEFT = 2

    @staticmethod
    def is_syslib(name: str) -> bool:
        return name.startswith('<') and name.endswith('>')

    @staticmethod
    def remove_ext(name: str) -> str:
        if utils.is_syslib(name):
            return name

        return os.path.splitext(name)[0]

# GRAPH manipulators


def transposed(g: Graph) -> Graph:
    out = Graph()
    for node, succs in g.get_dct().items():
        for succ in succs:
            out.add_dep(succ, node)

    return out


def removed_syslibs(g: Graph) -> Graph:
    out = Graph()

    for node, succs in g.get_dct().items():
        if utils.is_syslib(node):
            continue
        for succ in succs:
            if utils.is_syslib(succ):
                continue
            out.add_dep(node, succ)

    return out


def merge_headers_and_impl(g: Graph) -> Graph:
    out = Graph()

    for node in g.nodes():
        node_name = utils.remove_ext(node)
        for succ in g.successors(node):
            succ_name = utils.remove_ext(succ)

            # do not create one-node loop
            if node_name == succ_name:
                continue

            out.add_dep(node_name, succ_name)

    return out


# STATS computation

def has_cycle_rec(
        node: str,
        g: Graph,
        visited: dict[str, int],
) -> bool:
    assert node in visited, node

    if visited[node] != utils.dfs.NOT_SEEN:
        return visited[node] == utils.dfs.ENTERED

    visited[node] = utils.dfs.ENTERED

    for succ in g.successors(node):
        if has_cycle_rec(succ, g, visited):
            return True

    visited[node] = utils.dfs.LEFT
    return False


def has_cycle(g: Graph) -> bool:
    visited = {node: utils.dfs.NOT_SEEN for node in g.nodes()}

    for node in g.nodes():
        if has_cycle_rec(node, g, visited):
            return True

    return False


def get_shortest_cycles(g: Graph) -> list[list[str]]:
    cycles: list[list[str]] = list()
    for node in g.nodes():
        cycle = found_shortest_nonzero_path(g, node)
        if cycle is not None:
            cycles.append(cycle)

    return cycles


def found_shortest_nonzero_path(g: Graph, node: str) -> Optional[list[str]]:
    preds: dict[str, Optional[str]] = {n: None for n in g.nodes()}

    queue = deque([node])
    while queue:
        current = queue.pop()

        if current == node and preds[node] is not None:
            break

        for succ in g.successors(current):
            # already seen
            if preds[succ] is not None:
                continue

            preds[succ] = current
            queue.append(succ)

    if preds[node] is None:
        return None

    path = [node]
    curr = preds[node]
    while curr != node:
        assert type(curr) is str, curr
        path.append(curr)
        curr = preds[curr]

    path.append(node)
    return path[::-1]


def count_nodes_edges(g: Graph) -> tuple[int, int]:
    nodes = len(g.get_dct())
    edges = 0
    for node in g.get_dct():
        edges += len(g.successors(node))

    return nodes, edges


def get_layers(g: Graph) -> tuple[list[set[str]], set[str]]:
    out: list[set[str]] = []
    compiled: set[str] = set()

    # make system libs an independent layer
    layer = {node for node in g.nodes() if utils.is_syslib(node)}
    if layer:
        out.append(layer)
        compiled.update(layer)

    new_layer: Optional[set[str]] = None
    while new_layer != set():
        new_layer = set()
        for node in g.nodes():
            if node in compiled:
                continue

            for succ in g.successors(node):
                if succ not in compiled:
                    break
            else:
                # dependecies are compiled
                new_layer.add(node)

        if new_layer:
            out.append(new_layer)
        compiled.update(new_layer)

    return out, g.nodes().difference(compiled)


# PRINT statistics
def pprint_iter(lst: Iterable[Any], sep: str = ' ') -> None:
    buff: list[str] = []

    first = True

    def print_buff():
        nonlocal first
        print('\t\t', end='')
        for f in buff:
            print((sep if not first else '') + f'{f}', end='')
            first = False
        print()

    for elem in lst:
        buff.append(elem)
        if len(buff) > 3:
            print_buff()
            buff.clear()

    if buff:
        print_buff()


def print_cycles(g: Graph) -> None:
    got_cycle = has_cycle(g)
    print(f'has_cycle : {got_cycle}')
    if not got_cycle:
        return

    cycles = get_shortest_cycles(g)

    for i, cycle in enumerate(cycles):
        print(f'\tCycle {i}:')
        pprint_iter(cycle, ' -> ')

    print()


def print_layers(g: Graph) -> None:
    layers, uncompiled = get_layers(g)

    print('\nLayer decomposition:')
    for i, layer in enumerate(layers):
        print(f'\tLayer {i}, files {len(layer)}:')
        pprint_iter(layer, '  ')

    print(f'\tNot in layers:')
    pprint_iter(uncompiled, '  ')

    print(f'\tFound {sum(len(x) for x in layers)} files in layers')
    print(f'\t{len(uncompiled)} files were not in layers')

    print()
    print()


def print_stats(g: Graph) -> None:
    DOT_FOLDER = 'graph_dots'
    os.makedirs(DOT_FOLDER, exist_ok=True)

    def run_tests(name: str, gr: Graph):
        print(name.upper())
        path = os.path.join(DOT_FOLDER, name.replace(' ', '_') + '.dot')
        create_dot(gr, path)
        print(f'Visualisation saved in {path}')
        print('Counts ... nodes: {0}, edges: {1}'.format(
            *count_nodes_edges(gr))
        )

        print_cycles(gr)
        print_layers(gr)

    run_tests('original', g)
    run_tests('original without syslibs', removed_syslibs(g))
    run_tests('transposed', transposed(g))
    run_tests('transposed without syslibs', transposed(removed_syslibs(g)))
    run_tests('merged', merge_headers_and_impl(g))
    run_tests('merged without syslibs',
              merge_headers_and_impl(removed_syslibs(g))
              )


def main() -> None:
    parser = argparse.ArgumentParser()

    parser.add_argument(
        '-i',
        metavar="JSON",
        required=True,
        type=str,
        help='Path to json file with graph',
    )

    args = parser.parse_args()

    with open(args.i, 'r') as f:
        graph = Graph(json.load(f))

    print_stats(graph)


if __name__ == '__main__':
    main()
