dragonLoot = ['gold coin', 'dagger', 'gold coin', 'gold coin', 'ruby']

inv = {'gold coin':42, 'rope':1}

def dispInventory(inventory:dict):
    print('Inventory:')
    for k,v in inventory.items():
        print('%d %s' %(v, k))

def addToInventory(inventory:dict, addedItems:list):
    for i in dragonLoot:
        if i in inventory:
            inventory[i] += 1
        else:
            inventory.setdefault(i, 1)


dispInventory(inv)
addToInventory(inv, dragonLoot)
dispInventory(inv)