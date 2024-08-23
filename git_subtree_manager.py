import subprocess
import argparse
import json
from colorama import Fore, Style

error_code = 0


def run_command(command):
    """Runs a git command and returns the output."""
    global error_code
    result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if result.returncode != 0:
        print(Fore.RED + result.stderr + Style.RESET_ALL)
        error_code = result.returncode;
        raise Exception(f"{command} failed with exit code {result.returncode}.")
    return result.stdout


parser = argparse.ArgumentParser(description="Manage Git subtrees.")
parser.add_argument('command', choices=['add', 'pull', 'push', 'pull-all'],
                    help="Command to run: add, pull, push, or pull-all")
parser.add_argument('name', nargs='?', help="The name of the subtree (as defined in the config file)")
parser.add_argument('--config', default='subtrees.json',
                    help="The path to the configuration file (default: subtrees.json)")

args = parser.parse_args()


def load_config(config_file):
    """Loads the JSON configuration file."""
    with open(config_file, 'r') as file:
        return json.load(file)


config = load_config(args.config)

try:

    if args.command == 'pull-all':
        for name, subtree in config['subtrees'].items():
            print(f"\n--- Pulling updates for '{name}' ---")
            print(f"Pulling updates from remote branch: {subtree['branch']} into {subtree['prefix']}...")
            for repo_url in subtree['repo_urls']:
                try:
                    run_command(f'git subtree pull --prefix {subtree["prefix"]} {repo_url} {subtree["branch"]}')
                except Exception as e:
                    print(f"Error: {e}")
                    if error_code != 0:
                        exit(error_code)
    else:
        if args.name not in config['subtrees']:
            print(f"Error: Subtree '{args.name}' not found in configuration.")
        else:
            subtree = config['subtrees'][args.name]
            prefix = subtree['prefix']
            repo_url = subtree['repo_urls'][0]
            branch = subtree['branch']

            run_command(f'git subtree {args.command} --prefix {prefix} {repo_url} {branch}')

except Exception as e:
    print(f"Error: {e}")
