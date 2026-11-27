#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-2-Clause-Patent
"""Apply or verify the 21H461 Setup preferences on a mounted APFS Data seed."""
import argparse
import os
import plistlib
from pathlib import Path
from stat import S_IMODE


BUILD = '21H461'
MOBILE = 501
PREFERENCES = {
    'com.apple.migration': {
        'LastSystemVersion': BUILD,
        'DMUserDataDisposition': {'SyntheticDidUpgrade': False, 'buildVersion': BUILD},
    },
    'com.apple.springboard': {
        'BKNoWatchdogs': True,
        'FBLaunchWatchdogExceptions': {'com.apple.purplebuddy': 100.0},
        'MCForceShowCloudConfigurationUI': False,
        'SBDisableAutoDim': True,
        'SBDontDimOrLockOnAC': True,
        'SBDisableIdleSleep': True,
    },
    'com.apple.purplebuddy': {
        'SetupDone': True,
        'SetupFinishedAllSteps': True,
        'SetupVersion': 11,
        'daemonizeNeedsToRun': False,
    },
    'com.apple.purplebuddy.notbackedup': dict.fromkeys((
        'ShouldRunDiagnosticsMiniBuddy', 'ShouldRunHSA2MiniBuddy',
        'ShouldRunKeychainSyncMiniBuddy', 'ShouldRunMesaMiniBuddy',
        'ShouldRunPaymentMiniBuddy', 'ShouldRunStandardMiniBuddy',
        'SilentICDPUpgrade',
    ), False),
    'com.apple.managedconfiguration.notbackedup': {
        'LockdownActivationIndicatesCloudConfigurationAvailable': False,
    },
}
CONTAINERS = ('Bundle', 'Data/System', 'Shared/SystemGroup', 'Temp')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('data', type=Path, help='mount point of the copied Data volume')
    parser.add_argument('mode', choices=('write', 'verify'))
    args = parser.parse_args()
    prefs = args.data / 'mobile/Library/Preferences'
    if not prefs.is_dir():
        parser.error(f'{prefs} is missing from the Data seed')
    write = args.mode == 'write'
    for domain, values in PREFERENCES.items():
        path = prefs / f'{domain}.plist'
        data = plistlib.loads(path.read_bytes()) if path.exists() else {}
        if not isinstance(data, dict):
            raise ValueError(f'{path}: expected a preference dictionary')
        if write:
            data.update(values)
            path.write_bytes(plistlib.dumps(data, fmt=plistlib.FMT_BINARY))
            os.chown(path, MOBILE, MOBILE)
            path.chmod(0o600)
        else:
            for key, value in values.items():
                if key not in data or type(data[key]) is not type(value) or data[key] != value:
                    raise ValueError(f'{path}: unexpected value for {key}')
            info = path.stat()
            if (info.st_uid, info.st_gid, S_IMODE(info.st_mode)) != (MOBILE, MOBILE, 0o600):
                raise ValueError(f'{path}: expected owner 501:501 and mode 0600')
    for name in CONTAINERS:
        path = args.data / 'containers' / name
        if write:
            path.mkdir(parents=True, exist_ok=True)
            path.chmod(0o755)
        elif not path.is_dir() or S_IMODE(path.stat().st_mode) != 0o755:
            raise ValueError(f'{path}: expected a directory with mode 0755')


if __name__ == '__main__':
    main()
