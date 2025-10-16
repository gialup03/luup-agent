"""
Exception classes for luup-agent Python bindings.

Maps C error codes to Python exceptions for idiomatic error handling.
"""

from typing import Dict, Type


class LuupError(Exception):
    """Base exception for all luup-agent errors."""
    pass


class InvalidParameterError(LuupError):
    """Invalid parameter provided to a function."""
    pass


class OutOfMemoryError(LuupError):
    """Memory allocation failed."""
    pass


class ModelNotFoundError(LuupError):
    """Model file not found."""
    pass


class InferenceError(LuupError):
    """Inference operation failed."""
    pass


class ToolNotFoundError(LuupError):
    """Requested tool not registered with agent."""
    pass


class JsonParseError(LuupError):
    """JSON parsing failed."""
    pass


class HttpError(LuupError):
    """HTTP request failed."""
    pass


class BackendInitError(LuupError):
    """Backend initialization failed."""
    pass


# Map C error codes to Python exception classes
# Based on luup_error_t enum in luup_agent.h
ERROR_MAP: Dict[int, Type[LuupError]] = {
    -1: InvalidParameterError,
    -2: OutOfMemoryError,
    -3: ModelNotFoundError,
    -4: InferenceError,
    -5: ToolNotFoundError,
    -6: JsonParseError,
    -7: HttpError,
    -8: BackendInitError,
}


def check_error(error_code: int, get_error_msg_func) -> None:
    """
    Check C function return code and raise appropriate Python exception.
    
    Args:
        error_code: Return code from C function (0 = success, negative = error)
        get_error_msg_func: Function to get the last error message from C
        
    Raises:
        LuupError: Appropriate exception based on error code
    """
    if error_code == 0:  # LUUP_SUCCESS
        return
    
    # Get error message from C library
    error_msg = "Unknown error"
    try:
        msg_ptr = get_error_msg_func()
        if msg_ptr:
            error_msg = msg_ptr.decode('utf-8')
    except Exception:
        pass  # Use default message if we can't get the C error message
    
    # Raise appropriate exception
    exc_class = ERROR_MAP.get(error_code, LuupError)
    raise exc_class(f"[Error {error_code}] {error_msg}")

