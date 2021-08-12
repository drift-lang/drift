# build.py
import os
import sys

# Modify it to your GCC command
GCC = 'gcc'
DEBUG = len(sys.argv) != 1 and sys.argv[1] == '-bug'

list = []
path = './src/'

for f in os.listdir(path):
    if f.endswith('.c'):
        list.append(f)
for i, v in enumerate(list):
    print(i + 1, v)
    os.system(GCC + ' -std=gnu99 -c -Os -g %s' % path + v)
    list[i] = v[0:len(v) - 2] + '.o'
if DEBUG:
    os.system(GCC + ' -fsanitize=address %s -o drift' % ' '.join(list))
else:
    os.system(GCC + ' %s -o drift' % ' '.join(list))
os.system('rm -f *.o')
print('Done!', os.stat('drift').st_size / 1000, 'KB')
