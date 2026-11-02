#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Portable entry point for the required local pinned font oracle."""
from pathlib import Path
import runpy

runpy.run_path(str(Path(__file__).resolve().parents[1] / 'tests/test_font_pinned_oracle.py'),
               run_name='__main__')
