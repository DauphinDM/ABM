"""Exercise headless commands without touching the user's settings or skills."""
import json
import os
from pathlib import Path
import subprocess
import tempfile
import sys

binary = Path(os.environ.get('BEAMSIDE_BINARY', Path(__file__).resolve().parent.parent / ('build/release/abm.exe' if sys.platform == 'win32' else 'build/abm')))
scratch_parent = '/tmp/opencode' if Path('/tmp/opencode').is_dir() else None
with tempfile.TemporaryDirectory(prefix='beamdeck-cli-', dir=scratch_parent) as scratch:
    # Windows temp paths may contain 8.3 aliases while Qt returns long names.
    # Compare canonical paths, and still require the installed file inside HOME.
    root = Path(scratch).resolve()
    env = dict(os.environ, HOME=str(root), USERPROFILE=str(root), APPDATA=str(root / 'roaming'), LOCALAPPDATA=str(root / 'local'), XDG_CONFIG_HOME=str(root / 'config'), XDG_RUNTIME_DIR=str(root))
    env.update(BEAMSIDE_CONFIG_DIR=str(root/'config'), BEAMSIDE_RUNTIME_DIR=str(root/'runtime'))
    def run(*args, ok=True):
        result = subprocess.run([str(binary), *args], env=env, text=True, capture_output=True)
        assert (result.returncode == 0) == ok, (args, result.stdout, result.stderr)
        return json.loads(result.stdout) if ok else result
    assert run('preferences', '--units', 'mph', '--favorite', 'etk800', '--json') == {'metric': False, 'favorites': ['etk800']}
    assert run('preferences', '--json')['metric'] is False
    run('preferences', '--units', 'bad', '--json', ok=False)
    run('preferences', '--favorite', '../bad', '--json', ok=False)
    result = run('skill', 'install', '--json')
    target = Path(result['path']).resolve()
    assert target.is_relative_to(root) and target.is_file(), (target, root)
    run('skill', 'install', '--json')
    target.write_text('Existing user content')
    run('skill', 'install', '--json', ok=False)
    assert target.read_text() == 'Existing user content'
    state = run('status', '--json', ok=False)
    assert json.loads(state.stdout)['connected'] is False
    run('open', '--tab', 'invalid', '--screenshot', str(root / 'test.png'), '--demo', ok=False)
    status = run('launch-status', '--json')
    assert isinstance(status['gameRunning'], bool) and isinstance(status['beammpRunning'], bool)
    result = run('setup', '--user-folder', str(root / 'not-a-game'), '--json', ok=False)
    assert json.loads(result.stdout)['ok'] is False
print('Headless CLI tests passed')
