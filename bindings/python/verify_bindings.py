#!/usr/bin/env python3
"""
Verification script for Python bindings.

Tests that the package structure is correct and can be imported.
"""

import sys
from pathlib import Path

# Add package to path
package_dir = Path(__file__).parent
sys.path.insert(0, str(package_dir))

print("=" * 70)
print("luup-agent Python Bindings Verification")
print("=" * 70)
print()

# Test 1: Import package
print("✓ Test 1: Import package")
try:
    import luup_agent
    print(f"  ✓ Package imported successfully")
    print(f"  ✓ Version: {luup_agent.__version__}")
    print(f"  ✓ Version tuple: {luup_agent.__version_info__}")
except Exception as e:
    print(f"  ✗ Failed to import: {e}")
    sys.exit(1)

# Test 2: Import classes
print("\n✓ Test 2: Import classes")
try:
    from luup_agent import Model, Agent
    print(f"  ✓ Model class imported")
    print(f"  ✓ Agent class imported")
except Exception as e:
    print(f"  ✗ Failed to import classes: {e}")
    sys.exit(1)

# Test 3: Import exceptions
print("\n✓ Test 3: Import exceptions")
try:
    from luup_agent import (
        LuupError,
        ModelNotFoundError,
        InferenceError,
        ToolNotFoundError,
        JsonParseError,
        HttpError,
        BackendInitError,
    )
    print(f"  ✓ All exception classes imported")
except Exception as e:
    print(f"  ✗ Failed to import exceptions: {e}")
    sys.exit(1)

# Test 4: Check exports
print("\n✓ Test 4: Check package exports")
expected_exports = [
    "Model",
    "Agent",
    "LuupError",
    "ModelNotFoundError",
    "InferenceError",
    "ToolNotFoundError",
    "__version__",
    "__version_info__",
]
for export in expected_exports:
    if hasattr(luup_agent, export):
        print(f"  ✓ {export}")
    else:
        print(f"  ✗ Missing export: {export}")
        sys.exit(1)

# Test 5: Check native library loading
print("\n✓ Test 5: Check native library loading")
try:
    from luup_agent import _native
    print(f"  ✓ Native library loaded successfully")
    print(f"  ✓ Library path: {_native._find_library()}")
except Exception as e:
    print(f"  ⚠ Warning: Native library not loaded: {e}")
    print(f"  Note: This is expected if C library not built yet")
    print(f"  Run ./build.sh in project root to build C library")

# Test 6: File structure
print("\n✓ Test 6: Check file structure")
required_files = [
    "luup_agent/__init__.py",
    "luup_agent/_native.py",
    "luup_agent/model.py",
    "luup_agent/agent.py",
    "luup_agent/exceptions.py",
    "tests/conftest.py",
    "tests/test_model.py",
    "tests/test_agent.py",
    "tests/test_tools.py",
    "examples/basic_chat.py",
    "examples/tool_calling.py",
    "examples/async_streaming.py",
    "setup.py",
    "pyproject.toml",
    "README.md",
]
for file in required_files:
    file_path = package_dir / file
    if file_path.exists():
        print(f"  ✓ {file}")
    else:
        print(f"  ✗ Missing: {file}")
        sys.exit(1)

print()
print("=" * 70)
print("✅ All verification tests passed!")
print("=" * 70)
print()
print("Next steps:")
print("1. Build C library: cd ../.. && ./build.sh")
print("2. Install package: python3 -m pip install -e . (in a venv)")
print("3. Run tests: pytest tests/ -v")
print("4. Run examples: python3 examples/basic_chat.py <model-path>")
print()

