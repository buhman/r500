import sys

def print_error(filename, buf, e):
    assert len(e.args) == 2, e
    message, token = e.args
    lines = buf.splitlines()
    line = lines[token.line - 1]

    error_name = str(type(e).__name__)
    col_indent = ' ' * token.col
    col_pointer = '^' * len(token.lexeme)
    RED = "\033[0;31m"
    DEFAULT = "\033[0;0m"
    print(f'File: "{filename}", line {token.line}, column {token.col}\n', file=sys.stderr)
    sys.stderr.write('    ')
    wrote_default = False
    for i, c in enumerate(line.decode('utf-8')):
        if i == token.col:
            sys.stderr.write(RED)
        sys.stderr.write(c)
        if i == token.col + len(token.lexeme) - 1:
            wrote_default = True
            sys.stderr.write(DEFAULT)
    if not wrote_default:
        sys.stderr.write(DEFAULT)
    sys.stderr.write('\n')
    print(f"    {RED}{col_indent}{col_pointer}{DEFAULT}", file=sys.stderr)
    print(f'{RED}{error_name}{DEFAULT}: {message}', file=sys.stderr)
