#!/usr/bin/env nix-shell
#!nix-shell -i python3 -p python3Packages.pyserial
"""Unified tool to upload TOTP secrets to Sensor Watch.

Reads otpauth URIs, builds compact keys, and either prints echo commands
or writes them directly to a serial device.
"""

import argparse
import sys
import time
from urllib.parse import urlparse, urlencode, parse_qs, unquote, quote


def strip_params(uri):
    parsed = urlparse(uri)
    params = parse_qs(parsed.query, keep_blank_values=True)
    kept = {}
    if "issuer" in params:
        kept["issuer"] = params["issuer"][0]
    new_query = urlencode(kept)
    return parsed._replace(query=new_query).geturl()


def get_issuer(uri):
    parsed = urlparse(uri)
    params = parse_qs(parsed.query, keep_blank_values=True)
    if "issuer" in params:
        return params["issuer"][0]
    label = unquote(parsed.path).lstrip("/totp/")
    if ":" in label:
        return label.split(":")[0]
    return label


def resolve_collisions(entries):
    keys = {uri: issuer[:3] for uri, issuer in entries}
    changed = True
    while changed:
        changed = False
        uris = list(keys)
        for i in range(len(uris)):
            for j in range(i + 1, len(uris)):
                if keys[uris[i]].lower() == keys[uris[j]].lower():
                    a, b = uris[i], uris[j]
                    resolved = False
                    for pos in range(max(len(a), len(b))):
                        ca = a[pos] if pos < len(a) else ""
                        cb = b[pos] if pos < len(b) else ""
                        if ca.lower() != cb.lower():
                            keys[a] = keys[a][:-1] + ca
                            keys[b] = keys[b][:-1] + cb
                            changed = True
                            resolved = True
                            break
                    if not resolved:
                        print(f"Error: cannot generate unique keys for:\n  {a}\n  {b}", file=sys.stderr)
                        sys.exit(1)
    return keys


def get_secret(uri):
    parsed = urlparse(uri)
    params = parse_qs(parsed.query, keep_blank_values=True)
    if "secret" in params:
        return params["secret"][0]
    return None


def main():
    parser = argparse.ArgumentParser(description="Upload TOTP secrets to Sensor Watch")
    parser.add_argument("input_file", nargs="?", help="File with otpauth URIs (default: stdin)")
    parser.add_argument("--serial", metavar="DEVICE", help="Serial device to write to (e.g. /dev/ttyACM0)")
    args = parser.parse_args()

    # Read original URIs
    if args.input_file:
        with open(args.input_file) as f:
            lines = f.read().splitlines()
    else:
        lines = sys.stdin.read().splitlines()

    originals = []
    for line in lines:
        line = line.strip()
        if line:
            originals.append(line)

    if not originals:
        print("Error: no URIs found", file=sys.stderr)
        sys.exit(1)

    # Build stripped URI -> original URI mapping
    stripped_to_original = {}
    for uri in originals:
        stripped_to_original[strip_params(uri)] = uri

    # Build issuer index from stripped URIs
    entries = []
    seen = set()
    for stripped_uri in stripped_to_original:
        if stripped_uri in seen:
            print(f"Error: duplicate URI:\n  {stripped_uri}", file=sys.stderr)
            sys.exit(1)
        seen.add(stripped_uri)
        entries.append((stripped_uri, get_issuer(stripped_uri)))

    keys = resolve_collisions(entries)
    index = {keys[uri]: uri for uri, _ in entries}

    # Generate echo commands
    commands = []
    first = True
    for key, stripped_uri in index.items():
        original_uri = stripped_to_original[stripped_uri]
        secret = get_secret(original_uri)
        if secret is None:
            print(f"Error: no secret found in original URI for: {stripped_uri}", file=sys.stderr)
            sys.exit(1)
        uri = f"otpauth://totp/{quote(key)}?issuer={quote(key)}&secret={secret}"
        op = ">" if first else ">>"
        commands.append(f"echo '{uri}' {op} totp_uris.txt")
        first = False

    # Print summary table to stderr
    summary = []
    for key, stripped_uri in index.items():
        original_uri = stripped_to_original[stripped_uri]
        parsed = urlparse(original_uri)
        bare_uri = unquote(parsed.path).lstrip("/")
        summary.append((bare_uri, key))

    uri_width = max(len(uri) for uri, _ in summary)
    alias_width = max(len(alias) for _, alias in summary)
    header_uri = "URI"
    header_alias = "Alias"
    uri_width = max(uri_width, len(header_uri))
    alias_width = max(alias_width, len(header_alias))

    print(f"{'URI':<{uri_width}}  {'Alias':<{alias_width}}", file=sys.stderr)
    print(f"{'-' * uri_width}  {'-' * alias_width}", file=sys.stderr)
    for bare_uri, alias in summary:
        print(f"{bare_uri:<{uri_width}}  {alias:<{alias_width}}", file=sys.stderr)
    print(file=sys.stderr)

    # Output
    if args.serial:
        import serial
        with serial.Serial(args.serial, baudrate=19200, timeout=1) as ser:
            for cmd in commands:
                ser.write((cmd + "\r\n").encode())
                time.sleep(0.2)
        print(f"Wrote {len(commands)} commands to {args.serial}", file=sys.stderr)
    else:
        for cmd in commands:
            print(cmd)


if __name__ == "__main__":
    main()
