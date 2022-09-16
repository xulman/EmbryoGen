import sys


def expand_file(path: str):
    print(f'Processing {path} ... ', end='')
    changed = 0

    lines = open(path, 'r').readlines()

    MACRO_NAME = 'throw ERROR_REPORT'
    changing = True
    while changing:
        changing = False
        for row in range(len(lines)):
            if row >= len(lines):
                break
            line = lines[row].strip()

            if not line.startswith(f'{MACRO_NAME}('):
                continue

            changing = True
            total = line
            rows = [row]
            while ';' not in lines[row]:
                row += 1
                rows.append(row)
                total += ' ' + lines[row].strip()

            total = total.replace('" "', '')
            assert total.startswith(f'{MACRO_NAME}('), total
            assert total.endswith(');'), total

            total = total[len(f'{MACRO_NAME}('):-len(');')]
            parts = total.split('<<')
            string = ''
            args = []
            for part in parts:
                part = part.strip()
                if part.startswith('"'):
                    assert part.endswith('"'), part
                    string += part[1:-1]
                else:
                    string += '{}'
                    args.append(part)

            for _ in rows:
                lines.pop(rows[0])

            arg_string = ''
            for arg in args:
                arg_string += ', ' + arg

            lines.insert(
                rows[0], f'throw std::runtime_error(fmt::format("{{}} {string}", report::getIdnet() {arg_string}));\n')
            changed += 1

            break

    open(path, 'w').writelines(lines)
    if changed > 0:
        print(f'OK [{changed=}]')
    else:
        print('OK')


def main():
    if not sys.argv:
        print('Please enter paths to files as arguments')
        exit(1)

    files = sys.argv[1:]
    for file in files:
        expand_file(file)


if __name__ == '__main__':
    main()
