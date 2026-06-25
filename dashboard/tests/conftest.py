"""Shared fixtures: ensure the native library exists before binding tests."""

from __future__ import annotations

import os

import pytest

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")


@pytest.fixture(scope="session")
def lib():
    from rbmk_dash.core import bindings

    try:
        return bindings.load_library()
    except RuntimeError as exc:  # pragma: no cover - environment dependent
        pytest.skip(f"native library unavailable: {exc}")
