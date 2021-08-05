# build.py
import os

list = []
path = './src/'

for f in os.listdir(path):
    if f.endswith('.c'):
        list.append(f)
for i, v in enumerate(list):
    print(i + 1, v)
    os.system('gcc -std=gnu99 -c -Os -g %s' % path + v)
    list[i] = v[0:len(v) - 2] + '.o'
os.system('gcc %s -o drift' % ' '.join(list))
os.system('rm -f *.o')
print('Done!', os.stat('drift').st_size / 1000, 'KB')
