import os, sys
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'sphinx_extensions'))

from dfhack.changelog import cli_entrypoint
cli_entrypoint()
