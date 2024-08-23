import subprocess
import argparse
import json
from colorama import Fore, Style

error_code =0

def run_git_command(args):
    """Runs a git command and returns the output."""
    global error_code
    result = subprocess.run(['git'] + args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if result.returncode != 0:
        print(Fore.RED + result.stderr + Style.RESET_ALL)
        error_code = result.returncode;
        raise Exception(f"Command '{' '.join(args)}' failed with exit code {result.returncode}.")
    return result.stdout



def add_subtree(prefix, repo_url, branch):
    """Adds a Git subtree."""
    print(f"Adding subtree from {repo_url} to {prefix}...")
    output = run_git_command(['subtree', 'add', '--prefix', prefix, repo_url, branch])
    print(output)


def pull_subtree(prefix, repo_url, branch):
    """Pulls updates for a Git subtree."""
    print(f"Pulling updates from {repo_url} into {prefix}...")
    output = run_git_command(['subtree', 'pull', '--prefix', prefix, repo_url, branch])
    print(output)


def push_subtree(prefix, repo_url, branch):
    """Pushes local changes to the Git subtree back to the remote repo."""
    print(f"Pushing local changes in {prefix} to {repo_url}...")
    output = run_git_command(['subtree', 'push', '--prefix', prefix, repo_url, branch])
    print(output)

def try_urls_for_command(command_func, prefix, repo_urls, branch):
    """Tries each URL until a successful git command execution."""
    for repo_url in repo_urls:
        print(f"Trying with URL: {repo_url}")
        try:
            command_func(prefix, repo_url, branch)
            return
        except Exception as e:
            print(f"Error: {e}")
            if error_code != 0:
                exit(error_code)
    print("All repo URLs failed.")


def load_config(config_file):
    """Loads the JSON configuration file."""
    with open(config_file, 'r') as file:
        return json.load(file)


def pull_all_subtrees(subtrees):
    """Pulls updates for all Git subtrees."""
    for name, subtree in subtrees.items():
        print(f"\n--- Pulling updates for '{name}' ---")
        print(f"Pulling updates from remote branch: {subtree['branch']} into {subtree['prefix']}...")
        try_urls_for_command(pull_subtree, subtree['prefix'], subtree['repo_urls'], subtree['branch'])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Manage Git subtrees.")
    parser.add_argument('command', choices=['add', 'pull', 'push', 'pull-all'],
                        help="Command to run: add, pull, push, or pull-all")
    parser.add_argument('name', nargs='?', help="The name of the subtree (as defined in the config file)")
    parser.add_argument('--config', default='subtrees.json',
                        help="The path to the configuration file (default: subtrees.json)")

    args = parser.parse_args()
    config = load_config(args.config)

    if args.command == 'pull-all':
        pull_all_subtrees(config['subtrees'])
    else:
        if args.name not in config['subtrees']:
            print(f"Error: Subtree '{args.name}' not found in configuration.")
        else:
            subtree = config['subtrees'][args.name]
            if args.command == 'add':
                try_urls_for_command(add_subtree, subtree['prefix'], subtree['repo_urls'], subtree['branch'])
            elif args.command == 'pull':
                try_urls_for_command(pull_subtree, subtree['prefix'], subtree['repo_urls'], subtree['branch'])
            elif args.command == 'push':
                try_urls_for_command(push_subtree, subtree['prefix'], subtree['repo_urls'], subtree['branch'])
