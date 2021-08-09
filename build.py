# build.py
import os

# Modify it to your GCC command
GCC = 'gcc-11'

list = []
path = './src/'

for f in os.listdir(path):
    if f.endswith('.c'):
        list.append(f)
for i, v in enumerate(list):
    print(i + 1, v)
    os.system(GCC + ' -std=gnu99 -c -Os -g %s' % path + v)
    list[i] = v[0:len(v) - 2] + '.o'
os.system(GCC + ' %s -o drift' % ' '.join(list))
os.system('rm -f *.o')
print('Done!', os.stat('drift').st_size / 1000, 'KB')
