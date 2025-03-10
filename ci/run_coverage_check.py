#!/usr/bin/env python3
"""
@file run_coverage_check.py
@brief Run coverage checking.

@kaspersky_support Artiom N.
@license Apache 2.0 License.
@copyright © 2024 AO Kaspersky Lab
@date 28.10.2024.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
"""

import json
import subprocess
import sys
from pathlib import Path


if len(sys.argv) < 3:
    print(f'{Path(__file__).name} <root directory> <percent>')
    sys.exit(1)

root_dir = Path(sys.argv[1])
percent = int(sys.argv[2])
params = sys.argv[2:]

command = [
    'gcovr',
    '-r',
    str(root_dir),
    '-e',
    str(Path('.*usr') / 'include'),
    '-e',
    str(root_dir / 'third-party' / '.*'),
    '-e',
    str(root_dir / 'lib' / 'third-party' / '.*'),
    '-e',
    str(root_dir / '_deps' / '.*'),
    '-e',
    str(root_dir / 'knp' / '.*-traits-library'),
    '-e',
    str(root_dir / 'examples'),
    '--filter',
    'knp',
    '--json-summary',
    '--gcov-ignore-parse-errors',
]

result = subprocess.run(command, check=False, stdout=subprocess.PIPE)

if result.returncode != 0:
    sys.exit(result.returncode)

report = json.loads(result.stdout)
line_percent = int(report['line_percent'])

print(
    f'Total line count = {report["line_total"]}\n'
    f'Line coverage percentage = {line_percent}\n'
    f'Total function count = {report["function_total"]}\n'
    f'Function coverage percentage = {report["function_covered"]}\n'
    f'Total branch count = {report["branch_total"]}\n'
    f'Branch coverage percentage = {report["branch_percent"]}'
)

exit_code = percent - line_percent if line_percent < percent else 0

if exit_code:
    print(
        f'Warning: coverage analysis was not passed [{percent}% coverage is necessary, '
        f'but only {line_percent}% is covered]!'
    )
    files = sorted(report['files'], key=lambda k: int(k['line_total']))[:10]
    print('Top-10 files without coverage:')
    for f in files:
        print(f'  {f["filename"]} ({f["line_percent"]}%)')
else:
    print('Coverage analysis passed.')

sys.exit(exit_code)
