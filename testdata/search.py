lst=[3,2,5,4,1]
num=int(input("find? "))
for i in range(len(lst)):
    if lst[i] == num:
        print("found: {}".format(num))
