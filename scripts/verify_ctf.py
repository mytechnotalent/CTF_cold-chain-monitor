#!/usr/bin/env python3
"""Verify the OPERATION COLD IRON ACT-I artifacts against the solution.

Exits zero only when the shipped compromised image and the corrected
image match every documented offset and byte value, differ in exactly
the intended number of bytes, and carry the documented SHA-256 digests.
"""
import hashlib
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BUGGY = ROOT / "ACT-I.bin"
FIXED = ROOT / "ACT-I_fixed.bin"
BUGGY_SHA256 = (
    "b6fced1c0c305fdf167ed2da93464997504e1126d552eb120016d34c3a0bc47a"
)
FIXED_SHA256 = (
    "e63c513a8b798c0e4be9265e4313b8d882f3b6fd8e9ae1161316adad7a926e29"
)

CHANGES = (
    (0x759E, 0xFA, 0x00, "cold-offset breach threshold"),
    (0x754F, 0xD1, 0xD0, "dead annunciator branch"),
    (0x7686, 0x48, 0xFA, "damper seal pulse width"),
    (0x6569, 0xD0, 0xD1, "open infrared address gate"),
    (0x7AFF, 0xD1, 0xD0, "constant tag branch"),
    (0x7858, 0x00, 0x18, "nonce reuse loop bound"),
)


def _sha256(data: bytes) -> str:
    """
    Return the hexadecimal SHA-256 digest of a byte string.

    Parameters
    ----------
    data : bytes
        Byte string to hash.

    Returns
    -------
    str
        Lowercase hexadecimal digest.
    """
    return hashlib.sha256(data).hexdigest()


def _report(label: str, ok: bool) -> bool:
    """
    Print one labelled pass or fail result.

    Parameters
    ----------
    label : str
        Human-readable check label.
    ok : bool
        True when the check passed.

    Returns
    -------
    bool
        The same pass flag, for accumulation.
    """
    print(f"[{'PASS' if ok else 'FAIL'}] {label}")
    return ok


def _check_change(index: int, buggy: bytes, fixed: bytes) -> bool:
    """
    Check one documented offset and both byte values.

    Parameters
    ----------
    index : int
        Zero-based index into CHANGES.
    buggy : bytes
        Compromised image bytes.
    fixed : bytes
        Corrected image bytes.

    Returns
    -------
    bool
        True when the compromised and corrected bytes match the plan.
    """
    offset, want_buggy, want_fixed, label = CHANGES[index]
    ok = buggy[offset] == want_buggy and fixed[offset] == want_fixed
    return _report(f"0x{offset:04X} {label}", ok)


def _check_sizes(buggy: bytes, fixed: bytes) -> list[bool]:
    """
    Check that both images exist and share one size.

    Parameters
    ----------
    buggy : bytes
        Compromised image bytes.
    fixed : bytes
        Corrected image bytes.

    Returns
    -------
    list[bool]
        Existence and size check results.
    """
    return [
        _report("ACT-I.bin exists", BUGGY.exists()),
        _report("ACT-I_fixed.bin exists", FIXED.exists()),
        _report("sizes equal", len(buggy) == len(fixed)),
    ]


def _check_diff(buggy: bytes, fixed: bytes) -> bool:
    """
    Check that exactly the documented offsets differ.

    Parameters
    ----------
    buggy : bytes
        Compromised image bytes.
    fixed : bytes
        Corrected image bytes.

    Returns
    -------
    bool
        True when only the intended offset list changed.
    """
    pairs = zip(buggy, fixed)
    changed = [i for i, pair in enumerate(pairs) if pair[0] != pair[1]]
    expected = sorted(change[0] for change in CHANGES)
    return _report("exactly the intended bytes differ", changed == expected)


def _check_hashes(buggy: bytes, fixed: bytes) -> list[bool]:
    """
    Check both documented SHA-256 digests.

    Parameters
    ----------
    buggy : bytes
        Compromised image bytes.
    fixed : bytes
        Corrected image bytes.

    Returns
    -------
    list[bool]
        Digest check results.
    """
    return [
        _report("ACT-I.bin sha256", _sha256(buggy) == BUGGY_SHA256),
        _report("ACT-I_fixed.bin sha256", _sha256(fixed) == FIXED_SHA256),
    ]


def _summary(results: list[bool]) -> int:
    """
    Print the aggregate result and return the process status.

    Parameters
    ----------
    results : list[bool]
        Collected check results.

    Returns
    -------
    int
        Zero when every check passed, otherwise one.
    """
    total = sum(results)
    print(f"\n{total}/{len(results)} checks passed")
    return 0 if total == len(results) else 1


def main() -> int:
    """
    Run every ACT-I artifact check.

    Parameters
    ----------
    None

    Returns
    -------
    int
        Zero when every check passes, otherwise one.
    """
    buggy = BUGGY.read_bytes()
    fixed = FIXED.read_bytes()
    results = _check_sizes(buggy, fixed)
    results += [_check_change(i, buggy, fixed) for i in range(len(CHANGES))]
    results.append(_check_diff(buggy, fixed))
    results += _check_hashes(buggy, fixed)
    return _summary(results)


if __name__ == "__main__":
    sys.exit(main())
