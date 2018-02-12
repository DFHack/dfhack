import os, subprocess, sys

MAX_TRIES = 5

dfhack = 'Dwarf Fortress.exe' if sys.platform == 'win32' else './dfhack'
test_stage = 'test_stage.txt'

def get_test_stage():
    if os.path.isfile(test_stage):
        return open(test_stage).read().strip()
    return '0'

os.chdir(sys.argv[1])
if os.path.exists(test_stage):
    os.remove(test_stage)

tries = 0
while True:
    tries += 1
    stage = get_test_stage()
    print('Run #%i: stage=%s' % (tries, get_test_stage()))
    if stage == 'done':
        print('Done!')
        os.remove(test_stage)
        sys.exit(0)
    if tries > MAX_TRIES:
        print('Too many tries - aborting')
        sys.exit(1)

    os.system(dfhack)
