from typing import Optional

class Graph:
    def __init__(self, g: Optional[dict[str, set[str]]] = None):
        self.g = g if g is not None else {}

    def add_dep(self, fr: str, to: str) -> None:
        assert type(fr) is str, fr
        assert type(to) is str, to

        if fr not in self.g:
            self.g[fr] = set()
        if to not in self.g:
            self.g[to] = set()

        self.g[fr].add(to)
        
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