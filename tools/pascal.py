def tri(parents, depth):
    if depth == 20:
        return

    new = [1] * (len(parents) + 1)
    for i in range(1, len(parents)):
        new[i] = parents[i] + parents[i - 1]
    if len(new) % 2 == 1:
        div = sum(new)
        print(list(reversed([new[i] / div for i in range(len(new) // 2 + 1)])))
    tri(new, depth + 1)

parents = [1, 2, 1]
tri(parents, 2)
