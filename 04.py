spam = ['apples', 'bananas', 'tofu', 'cats', 1]

def func(someList: list):
    str1 = ''
    someList[-1] = 'and ' + str(someList[-1])
    for num in range( len(someList) ):
        str1 = str1 + str(someList[num])
        if someList[num] != someList[-1]:
            str1 += ','
    print(str1)

func(spam)

grid = [
        ['.', '.', '.', '.', '.', '.'],
        ['.', 'O', 'O', '.', '.', '.'],
        ['O', 'O', 'O', 'O', '.', '.'],
        ['O', 'O', 'O', 'O', 'O', '.'],
        ['.', 'O', 'O', 'O', 'O', 'O'],
        ['O', 'O', 'O', 'O', 'O', '.'],
        ['O', 'O', 'O', 'O', '.', '.'],
        ['.', 'O', 'O', '.', '.', '.'],
        ['.', '.', '.', '.', '.', '.']]

h = len(grid)
l = len(grid[0])

print(len(grid), h, l)

for y in range(l):
    for x in range(h-1, -1, -1):
        print(grid[x][y], end='')
    print('')
    
