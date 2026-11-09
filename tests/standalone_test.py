"""Check the app-owned receiver with fake HOME and a private, non-game UDP port."""
import json
import os
from pathlib import Path
import socket
import subprocess
import sys
import tempfile
import time

binary = Path(os.environ.get('BEAMSIDE_BINARY', Path(__file__).resolve().parent.parent / 'build/abm'))
parent = '/tmp/opencode' if Path('/tmp/opencode').is_dir() else None
with tempfile.TemporaryDirectory(prefix='beamside-standalone-', dir=parent) as scratch:
    root = Path(scratch)
    env = dict(os.environ, HOME=scratch, USERPROFILE=scratch, APPDATA=str(root/'roaming'), LOCALAPPDATA=str(root/'local'),
               XDG_CONFIG_HOME=str(root/'config'), XDG_CACHE_HOME=str(root/'cache'), XDG_RUNTIME_DIR=scratch,
               QT_QPA_PLATFORM='offscreen', QT_QUICK_BACKEND='software', QT_FORCE_STDERR_LOGGING='1')
    env.update(BEAMSIDE_CONFIG_DIR=str(root/'config'), BEAMSIDE_RUNTIME_DIR=str(root/'runtime'))
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as reservation:
        reservation.bind(('127.0.0.1', 0))
        port = reservation.getsockname()[1]
    with (root/'app.log').open('w+') as log:
        app = subprocess.Popen([str(binary), 'open', '--receiver-port', str(port)], env=env, stdout=log, stderr=log)
        try:
            deadline = time.monotonic() + 15
            while time.monotonic() < deadline:
                assert app.poll() is None, 'Standalone app exited during startup'
                # Qt uses named pipes on Windows, not the Unix filesystem
                # socket probed below. State publication gates the startup;
                # the second process then verifies real activation over Qt IPC.
                if sys.platform == 'win32':
                    state_path = root / 'runtime' / 'state.json'
                    if state_path.is_file():
                        break
                    time.sleep(.2)
                    continue
                gui_socket = root / 'runtime' / 'dashboard.sock'
                try:
                    probe = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
                    probe.settimeout(1)
                    probe.connect(str(gui_socket))
                    probe.close()
                    break
                except OSError:
                    probe.close()
                    time.sleep(.2)
            else:
                raise AssertionError('Activation socket never appeared')
            state_files = list(root.rglob('state.json'))
            assert state_files and json.loads(state_files[0].read_text()).get('kind') == 'state', 'Standalone receiver did not publish a snapshot'
            second = subprocess.run([str(binary), 'open', '--tab', 'garage', '--receiver-port', str(port)], env=env, capture_output=True, timeout=8)
            assert second.returncode == 0, second.stderr.decode(errors='replace')
            assert app.poll() is None
            time.sleep(.3)
        finally:
            app.terminate()
            try:
                app.wait(timeout=8)
            except subprocess.TimeoutExpired:
                app.kill()
                app.wait(timeout=5)
        log.seek(0)
        output = log.read()
        assert 'failed to load component' not in output.lower(), output
        assert 'binding loop' not in output.lower(), output
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as check:
        check.bind(('127.0.0.1', port))
print('Standalone receiver startup, activation and shutdown tests passed')
