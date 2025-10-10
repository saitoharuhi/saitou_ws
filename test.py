a = 0
while a <= 10:

    if a % 2 == 0:
        if a == 10:
            print(a,"は偶数")
        else:
            print(a,"は偶数",end=",")       
    else:
        print(a,"は奇数",end=",")
    a = a + 1
        