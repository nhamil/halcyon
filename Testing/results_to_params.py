import sys

file = sys.argv[1]
maxiter = int(sys.argv[2])

to_c = False
if len(sys.argv) >= 4: 
    to_c = sys.argv[3].lower() == "true"

p = []

with open(file)  as f:
    lines = f.readlines()

for line in lines:
    words = line.split(" ")
    param = -1
    if words[0] == "Iteration" and int(words[1]) > maxiter:
        break
    for i in range(len(words) ):
        if words[i] == "param":
            param = int(words[i+1])
            while len(p) < param + 1:
                p.append(0)
        if words[i].startswith("(E"): 
            param = -1 
        elif words[i].startswith("(") and param >= 0: 
            p[param] = int(words[i][1:-1])
        elif words[i] == "to" and param >= 0:
            p[param] = int(words[i + 1])
            param = -1

print(" ".join(str(x) for x in p))

pi = 0
def nextp(): 
    global pi 
    out = p[pi] 
    pi += 1 
    return out 

def print_param_var(name, dim): 
    if len(dim) == 0: 
        print(f"int {name} = {nextp()};\n")
    elif len(dim) == 1: 
        print(f"int {name}[{dim[0]}] = \n{{", end='')
        for i in range(dim[0]): 
            if i % 8 == 0: 
                print("\n    ", end='')
            print(f"{nextp():4d}, ", end='')
        print("\n};\n") 
    elif len(dim) == 3: 
        print(f"int {name}[{dim[0]}][{dim[1]}][{dim[2]}] = \n{{", end='')
        for i in range(dim[0]): 
            print("\n    {", end='')
            for j in range(dim[1]): 
                print("\n        {", end='')
                for k in range(dim[2]): 
                    if k % 8 == 0: 
                        print("\n            ", end='')
                    print(f"{nextp():4d}, ", end='')
                print("\n        },", end='') 
            print("\n    },", end='') 
        print("\n};\n") 

if to_c: 
    print() 
    print_param_var("BishopPair", []) 
    print_param_var("PawnStructureValues", [4])
    print_param_var("PcTypeValues", [5]) 
    print_param_var("AttackUnitValues", [64])
    print_param_var("PcSq", [2, 6, 64]) 
