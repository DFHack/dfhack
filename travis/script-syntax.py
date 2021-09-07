#!/usr/bin/env python3

import os
import subprocess
import sys

script_name = os.path.basename(__file__)
new_script_path = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'ci', script_name)

sys.stderr.write('\nNote: travis/{script_name} is deprecated. Use ci/{script_name} instead.\n\n'.format(script_name=script_name))
sys.stderr.flush()

p = subprocess.run([sys.executable, new_script_path] + sys.argv[1:])
sys.exit(p.returncode)
