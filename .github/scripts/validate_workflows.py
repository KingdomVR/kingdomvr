import yaml
import sys

def check(path):
    try:
        with open(path, 'r', encoding='utf-8') as f:
            yaml.safe_load(f)
        print(f'OK: {path}')
    except yaml.YAMLError as e:
        print(f'ERROR: {path}')
        print(e)
    except Exception as e:
        print(f'UNEXPECTED ERROR ({path}): {e}')

if __name__ == '__main__':
    files = [
        '.github/workflows/build-windows-deps.yml',
        '.github/workflows/build-windows-from-store.yml'
    ]
    for p in files:
        check(p)
