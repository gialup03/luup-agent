"""
Native C bindings for luup-agent using ctypes.

This module loads the shared library and declares all C functions with proper types.
"""

import ctypes
import sys
import os
from pathlib import Path
from typing import Optional

# ============================================================================
# Library Loading
# ============================================================================


def _find_library() -> str:
    """
    Locate the luup-agent shared library.
    
    Returns:
        Path to the shared library
        
    Raises:
        FileNotFoundError: If library cannot be found
    """
    # Determine library extension based on platform
    if sys.platform == "darwin":
        lib_name = "libluup_agent.dylib"
    elif sys.platform == "win32":
        lib_name = "luup_agent.dll"
    else:  # Linux and other Unix
        lib_name = "libluup_agent.so"
    
    # Search locations (in order of priority)
    search_paths = [
        # 1. Package data (for installed package)
        Path(__file__).parent / lib_name,
        # 2. Build directory (for development)
        Path(__file__).parent.parent.parent.parent / "build" / lib_name,
        # 3. Environment variable
        os.environ.get("LUUP_LIBRARY_PATH", ""),
        # 4. System library paths
        lib_name,  # Let ctypes search system paths
    ]
    
    for path in search_paths:
        if path and Path(path).exists():
            return str(path)
    
    # If not found, try just the name and let the system find it
    return lib_name


# Load the shared library
try:
    _lib = ctypes.CDLL(_find_library())
except OSError as e:
    raise ImportError(
        f"Failed to load luup-agent library. "
        f"Make sure it's built and available. Error: {e}"
    ) from e


# ============================================================================
# C Structure Definitions
# ============================================================================


class CModelConfig(ctypes.Structure):
    """C structure: luup_model_config"""
    _fields_ = [
        ("path", ctypes.c_char_p),
        ("gpu_layers", ctypes.c_int),
        ("context_size", ctypes.c_int),
        ("threads", ctypes.c_int),
        ("api_key", ctypes.c_char_p),
        ("api_base_url", ctypes.c_char_p),
    ]


class CModelInfo(ctypes.Structure):
    """C structure: luup_model_info"""
    _fields_ = [
        ("backend", ctypes.c_char_p),
        ("device", ctypes.c_char_p),
        ("gpu_layers_loaded", ctypes.c_int),
        ("memory_usage", ctypes.c_size_t),
        ("context_size", ctypes.c_int),
    ]


class CAgentConfig(ctypes.Structure):
    """C structure: luup_agent_config"""
    _fields_ = [
        ("model", ctypes.c_void_p),
        ("system_prompt", ctypes.c_char_p),
        ("temperature", ctypes.c_float),
        ("max_tokens", ctypes.c_int),
        ("enable_tool_calling", ctypes.c_bool),
        ("enable_history_management", ctypes.c_bool),
        ("enable_builtin_tools", ctypes.c_bool),
    ]


class CTool(ctypes.Structure):
    """C structure: luup_tool"""
    _fields_ = [
        ("name", ctypes.c_char_p),
        ("description", ctypes.c_char_p),
        ("parameters_json", ctypes.c_char_p),
    ]


# ============================================================================
# C Callback Types
# ============================================================================

# Tool callback: char* (*)(const char* params_json, void* user_data)
CToolCallback = ctypes.CFUNCTYPE(
    ctypes.c_char_p,  # return type
    ctypes.c_char_p,  # params_json
    ctypes.c_void_p   # user_data
)

# Stream callback: void (*)(const char* token, void* user_data)
CStreamCallback = ctypes.CFUNCTYPE(
    None,             # return type (void)
    ctypes.c_char_p,  # token
    ctypes.c_void_p   # user_data
)

# Error callback: void (*)(luup_error_t code, const char* msg, void* user_data)
CErrorCallback = ctypes.CFUNCTYPE(
    None,             # return type (void)
    ctypes.c_int,     # error code
    ctypes.c_char_p,  # message
    ctypes.c_void_p   # user_data
)


# ============================================================================
# Error Handling Functions
# ============================================================================

_lib.luup_get_last_error.argtypes = []
_lib.luup_get_last_error.restype = ctypes.c_char_p

_lib.luup_set_error_callback.argtypes = [CErrorCallback, ctypes.c_void_p]
_lib.luup_set_error_callback.restype = None


# ============================================================================
# Model Layer Functions
# ============================================================================

_lib.luup_model_create_local.argtypes = [ctypes.POINTER(CModelConfig)]
_lib.luup_model_create_local.restype = ctypes.c_void_p

_lib.luup_model_create_remote.argtypes = [ctypes.POINTER(CModelConfig)]
_lib.luup_model_create_remote.restype = ctypes.c_void_p

_lib.luup_model_warmup.argtypes = [ctypes.c_void_p]
_lib.luup_model_warmup.restype = ctypes.c_int

_lib.luup_model_get_info.argtypes = [ctypes.c_void_p, ctypes.POINTER(CModelInfo)]
_lib.luup_model_get_info.restype = ctypes.c_int

_lib.luup_model_destroy.argtypes = [ctypes.c_void_p]
_lib.luup_model_destroy.restype = None


# ============================================================================
# Agent Layer Functions
# ============================================================================

_lib.luup_agent_create.argtypes = [ctypes.POINTER(CAgentConfig)]
_lib.luup_agent_create.restype = ctypes.c_void_p

_lib.luup_agent_register_tool.argtypes = [
    ctypes.c_void_p,
    ctypes.POINTER(CTool),
    CToolCallback,
    ctypes.c_void_p
]
_lib.luup_agent_register_tool.restype = ctypes.c_int

_lib.luup_agent_generate_stream.argtypes = [
    ctypes.c_void_p,
    ctypes.c_char_p,
    CStreamCallback,
    ctypes.c_void_p
]
_lib.luup_agent_generate_stream.restype = ctypes.c_int

_lib.luup_agent_generate.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
_lib.luup_agent_generate.restype = ctypes.c_void_p  # Return raw pointer for manual memory management

_lib.luup_agent_add_message.argtypes = [
    ctypes.c_void_p,
    ctypes.c_char_p,
    ctypes.c_char_p
]
_lib.luup_agent_add_message.restype = ctypes.c_int

_lib.luup_agent_clear_history.argtypes = [ctypes.c_void_p]
_lib.luup_agent_clear_history.restype = ctypes.c_int

_lib.luup_agent_get_history_json.argtypes = [ctypes.c_void_p]
_lib.luup_agent_get_history_json.restype = ctypes.c_char_p

_lib.luup_agent_enable_builtin_todo.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
_lib.luup_agent_enable_builtin_todo.restype = ctypes.c_int

_lib.luup_agent_enable_builtin_notes.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
_lib.luup_agent_enable_builtin_notes.restype = ctypes.c_int

_lib.luup_agent_enable_builtin_summarization.argtypes = [ctypes.c_void_p]
_lib.luup_agent_enable_builtin_summarization.restype = ctypes.c_int

_lib.luup_agent_destroy.argtypes = [ctypes.c_void_p]
_lib.luup_agent_destroy.restype = None


# ============================================================================
# Memory Management Functions
# ============================================================================

_lib.luup_free_string.argtypes = [ctypes.c_void_p]
_lib.luup_free_string.restype = None


# ============================================================================
# Version Information Functions
# ============================================================================

_lib.luup_version.argtypes = []
_lib.luup_version.restype = ctypes.c_char_p

_lib.luup_version_components.argtypes = [
    ctypes.POINTER(ctypes.c_int),
    ctypes.POINTER(ctypes.c_int),
    ctypes.POINTER(ctypes.c_int)
]
_lib.luup_version_components.restype = None


# ============================================================================
# Helper Functions
# ============================================================================


def get_version() -> str:
    """Get library version string."""
    return _lib.luup_version().decode('utf-8')


def get_version_tuple() -> tuple[int, int, int]:
    """Get library version as tuple (major, minor, patch)."""
    major = ctypes.c_int()
    minor = ctypes.c_int()
    patch = ctypes.c_int()
    _lib.luup_version_components(
        ctypes.byref(major),
        ctypes.byref(minor),
        ctypes.byref(patch)
    )
    return (major.value, minor.value, patch.value)


# Expose library object for advanced users
__all__ = [
    '_lib',
    'CModelConfig',
    'CModelInfo',
    'CAgentConfig',
    'CTool',
    'CToolCallback',
    'CStreamCallback',
    'CErrorCallback',
    'get_version',
    'get_version_tuple',
]

