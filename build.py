# build.py
import os
import sys

# Modify it to your GCC command
GCC = "gcc"
DEBUG = len(sys.argv) != 1 and sys.argv[1] == "-bug"
PREFIX = ""

list = []
path = "./src/"

for f in os.listdir(path):
    if f.endswith(".c"):
        list.append(f)
for i, v in enumerate(list):
    print(i + 1, v)
    os.system(GCC + " -std=c99 -l. -c -g %s" % path + v)
    list[i] = v[0 : len(v) - 2] + ".o"
if DEBUG:
    PREFIX = "-fsanitize=address"

os.system(GCC + " %s %s -Wl,-E -ldl -o drift" % (PREFIX, " ".join(list)))
os.system("rm -f *.o")
print("Done!", os.stat("drift").st_size / 1000, "KB")
