import sys
import os
from os import path
import argparse
import json
from typing import Optional


class Injector:
    string = ''

    @staticmethod
    def inject(s: str) -> None:
        Injector.string += s.strip('\n') + '\n\n'


class utils:
    class dfs:
        NOT_SEEN = 0
        ENTERED = 1
        LEFT = 2

    @staticmethod
    def is_syslib(name: str) -> bool:
        return name.startswith('<') and name.endswith('>')

    @staticmethod
    def remove_syslibs(names: list[str]) -> list[str]:
        return list(filter(lambda x: not utils.is_syslib(x), names))

    @staticmethod
    def remove_ext(name: str) -> str:
        if utils.is_syslib(name):
            return name

        return os.path.splitext(name)[0]

    @staticmethod
    def get_module_name(file: str) -> Optional[str]:
        with open(file, 'r') as f:
            for line in f:
                line = line.strip()
                if line.startswith('module ') and line[-1] == ';':
                    name = line[len('module'):-1].strip()
                    if name != '':
                        return name

                if line.startswith('export module ') and line[-1] == ';':
                    return line[len('export module'):-1].strip()

        return None

    @staticmethod
    def get_imports(file: str) -> list[str]:
        out: list[str] = []
        with open(file, 'r') as f:
            for line in f:
                line = line.strip()
                if line.startswith('import ') and line[-1] == ';':
                    name = line[len('import'):-1].strip()
                    if name != '':
                        out.append(name)

        return out

    @staticmethod
    def pretty_string(lst: list[str], bucket_size=5) -> str:
        out = ''
        bucket: list[str] = []

        def try_use_bucket(force=False):
            nonlocal out
            nonlocal bucket
            if (len(bucket) >= bucket_size)\
                    or len(bucket) != 0 and force:
                out += ' '.join(bucket) + '\n'
                bucket.clear()

        for elem in lst:
            bucket.append(elem)
            try_use_bucket()

        try_use_bucket(True)

        return out


class Graph:
    def __init__(self, g: Optional[dict[str, set[str]]] = None):
        self.g = g if g is not None else {}

    def add_dep(self, fr: str, to: str) -> None:
        assert type(fr) is str, fr
        assert type(to) is str, to

        self.add_node(fr)
        self.add_node(to)

        self.g[fr].add(to)

    def add_node(self, name: str) -> None:
        assert type(name) is str, name
        if not name in self.g:
            self.g[name] = set()

    def nodes(self) -> set[str]:
        return set(self.g.keys())

    def successors(self, node: str) -> set[str]:
        assert node in self.g, node
        return self.g[node]

    def get_dct(self) -> dict[str, set[str]]:
        return self.g

    def get_json_dct(self) -> dict[str, list[str]]:
        out = {}
        for key, value in self.g.items():
            out[key] = list(value)

        return out


def all_files(prefix: str = '.', src: str = 'src/') -> tuple[list[str], list[str], list[str]]:
    """Return core_files, headers and modules"""
    core_files: list[str] = []
    headers: list[str] = []
    modules: list[str] = []

    for entry in os.scandir(src):
        if 'tests' in entry.name:
            continue

        if entry.is_file():
            if path.splitext(entry.name)[-1] in {'.cpp', '.cc'}:
                core_files.append(path.join(prefix, entry.name))

            if path.splitext(entry.name)[-1] in {'.hpp', '.h'}:
                headers.append(path.join(prefix, entry.name))

            if path.splitext(entry.name)[-1] in {'.cxx'}:
                modules.append(path.join(prefix, entry.name))

        elif entry.is_dir():
            new_cf, new_h, new_m = all_files(
                path.join(prefix, entry.name),
                path.join(src, entry.name)
            )
            core_files += new_cf
            headers += new_h
            modules += new_m

    return core_files, headers, modules


def parse_import(row: str) -> str:
    row = row.strip()
    assert row.endswith(';'), row
    assert row.startswith('import'), row

    row = row[len('import'): -1]
    row = row.strip()
    return row


def split_sys_user_files(files: list[str]) -> tuple[list[str], list[str]]:
    syslibs: list[str] = []
    user: list[str] = []

    for file in files:
        if utils.is_syslib(file):
            syslibs.append(file)
        else:
            user.append(file)

    return syslibs, user


def prepend_src(files: list[str]) -> list[str]:
    out: list[str] = []
    for file in files:
        out.append(path.normpath(path.join('src', file)))
    return out


def get_module_files_map(module_files: list[str]) -> dict[str, list[str]]:
    print('Mapping files to module names and vice versa ... ', end='', flush=True)
    module_files_map: dict[str, list[str]] = {}

    for file in module_files:
        mod_name = utils.get_module_name(file)
        if mod_name is None:
            continue

        if not mod_name in module_files_map:
            module_files_map[mod_name] = []

        module_files_map[mod_name].append(file)

    print(f'OK [processed {len(module_files_map)} modules]')
    return module_files_map


def get_import_dep_graph(module_files_map: dict[str, list[str]]) -> Graph:
    print('Creating import dependency graph ... ', end='', flush=True)
    g = Graph()
    for mod, files in module_files_map.items():
        g.add_node(mod)
        for file in files:
            for imp in utils.remove_syslibs(utils.get_imports(file)):
                g.add_dep(mod, imp)

    print('OK')

    return g


def get_layers(g: Graph) -> list[set[str]]:
    print('Sorting modules to layers ... ', end='', flush=True)
    layers: list[set[str]] = []

    processed: set[str] = set()
    new_layer = set(['while condition is satisfied at first'])
    while new_layer:
        new_layer = set()
        for node in g.nodes():
            if node in processed:
                continue

            if all(succ in processed for succ in g.successors(node)):
                new_layer.add(node)

        if new_layer:
            layers.append(new_layer)

        processed.update(new_layer)

    assert sum(len(layer) for layer in layers) == len(g.nodes()),\
        'Some nodes are not in layers (probably caused by dependecy loop)'

    print(f'OK [created {len(layers)} layers]')
    return layers


def prepare_system_modules(all_files: list[str]) -> None:
    print('Searching for required modules of system library ... ', end='', flush=True)
    imports = []
    for file in all_files:
        imports.extend(utils.get_imports(file))

    syslibs, _ = split_sys_user_files(imports)
    syslibs = list(set(syslibs))

    s = """
set(STD_MODULES """ + ' '.join(map(lambda x: x[1:-1], syslibs)) + """)
add_custom_target(std_modules
    COMMAND ${CMAKE_COMMAND} -E echo "Building system libraries"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

foreach(module IN LISTS STD_MODULES)
    add_custom_command(TARGET std_modules PRE_BUILD 
        COMMAND ${CMAKE_COMMAND} -E echo "${module}"
        COMMAND g++ -fmodules-ts -O3 -std=c++20 -c -x c++-system-header ${module}
        )
endforeach()
"""
    Injector.inject(s)

    print(f'OK [found {len(syslibs)}]')


def inject_layers(layers: list[set[str]], module_files_map: dict[str, list[str]]) -> list[str]:
    """Return all libraries to depend on, created by layers and std modules"""
    print('Preparing layer injection string ... ', end='', flush=True)

    deps = ['std_modules']
    for i, layer in enumerate(layers, 1):
        files: list[str] = []
        for module in layer:
            files.extend(module_files_map[module])

        s = f"""
set(LAYER{i}_FILES { utils.pretty_string(files) })
add_library(layer{i} MODULE ${{LAYER{i}_FILES}})
add_dependencies(layer{i} {utils.pretty_string(deps)})
set_target_properties(layer{i} PROPERTIES CXX_STANDARD 20)
set_target_properties(layer{i} PROPERTIES COMPILE_FLAGS -fmodules-ts)
"""
        Injector.inject(s)
        deps.append(f'layer{i}')

    print('OK')
    return deps


def inject_dependencies(sources: list[str], deps: list[str]) -> None:
    sources_str = '\n'.join(sources)
    deps_str = '\n'.join(deps)
    Injector.inject(f'set(SOURCES ${{SOURCES}} \n{sources_str}\n)')
    Injector.inject(f'set(DEPS \n{deps_str}\n)')


def inject_cmake() -> None:
    print('Injecting CMakeLists.txt ... ', end='', flush=True)

    cmake_lines = open('CMakeLists.txt', 'r').readlines()
    INJ_START_LINE = '#---PYTHON CMAKE INJECTION START---'
    INJ_END_LINE = '#---PYTHON CMAKE INJECTION END---'

    # detect injection place in cmake
    inj_start = -1
    inj_end = -1

    for i, line in enumerate(cmake_lines):
        line = line.strip()
        if line == INJ_START_LINE:
            assert inj_start == -1, 'Multiple starting points found'
            inj_start = i

        if line == INJ_END_LINE:
            assert inj_end == -1, 'Multiple ending points found'
            inj_end = i

    assert inj_start != -1, 'No starting point found, make sure your CMakelists.txt contains line:\n' +\
        INJ_START_LINE

    assert inj_end != -1, 'No ending point found, make sure your CMakelists.txt contains line:\n' +\
        INJ_END_LINE

    assert inj_start < inj_end, 'Starting point must be before ending point\n'

    inj_lines: list[str] = []
    for line in Injector.string.splitlines():
        inj_lines.append(line + '\n')

    # remove content between points
    cmake_lines = cmake_lines[:inj_start + 1] + \
        inj_lines + cmake_lines[inj_end:]

    # write to CMakeLists.txt
    open('CMakeLists.txt', 'w').writelines(cmake_lines)

    print('OK')


def print_help() -> None:
    print("""
Run script in the directory with CMakeLists.txt
ALL source code must be included in ./src/ folder

RUN OPTIONS:
'without args'    - inject cmake with dependencies

help            - prints this help message
clean           - remove all content inside injection area in CMakeLists.txt
""")


def main() -> None:
    wrong_arg_msg = 'Wrong arguments, use python ./script.py help'
    if len(sys.argv) != 1:
        if len(sys.argv) == 2:
            if sys.argv[1] == 'clean':
                inject_cmake()
                return

            elif sys.argv[1] == 'help':
                print_help()
                return

            else:
                assert False, wrong_arg_msg

        else:
            assert False, wrong_arg_msg

    cores, headers, modules = all_files()
    cores = prepend_src(cores)
    headers = prepend_src(headers)
    modules = prepend_src(modules)

    prepare_system_modules(cores + headers + modules)
    module_files_map = get_module_files_map(modules)
    g = get_import_dep_graph(module_files_map)
    layers = get_layers(g)
    # deps = inject_layers(layers, module_files_map)
    # remove need of libraries, serialize output
    modules_in_order: list[str] = []
    for layer in layers:
        for mod in layer:
            modules_in_order.extend(module_files_map[mod])

    inject_dependencies(modules_in_order + cores, ['std_modules'])
    inject_cmake()

    print('Process Completed', end='\n\n')
    print('Please make sure to add ${SOURCES} as source for main target')
    print('Please add following line to your CMakeLists.txt:')
    print('add_dependencies([your_target_name] ${DEPS})')


if __name__ == '__main__':
    main()
