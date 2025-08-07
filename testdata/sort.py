def sort(lst):
    for i in range(len(lst)):
        m = i
        for j in range(i+1, len(lst)):
            if lst[j] < lst[m]:
                m = j
        (lst[i], lst[m]) = (lst[m], lst[i])

l = list()
cur = "1"
while True:
    cur = input("> ").strip()
    if len(cur) == 0:
        break
    num = int(cur)
    l.append(num)

sort(l)
for i, n in enumerate(l):
    if i != len(l) - 1:
        print(n, end=", ")
    else:
        print(n)
