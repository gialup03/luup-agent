"""
Model class for LLM inference.

Provides a Pythonic wrapper around the C model API with context management
and automatic resource cleanup.
"""

import atexit
from pathlib import Path
from typing import Optional, Dict, Any, Literal

# For Self type (Python 3.11+)
try:
    from typing import Self
except ImportError:
    from typing_extensions import Self

from . import _native
from .exceptions import check_error, ModelNotFoundError, BackendInitError


class Model:
    """
    LLM model for inference.
    
    Supports both local models (GGUF files via llama.cpp) and remote models
    (OpenAI-compatible APIs).
    
    Examples:
        >>> # Local model
        >>> model = Model.from_local("models/qwen-0.5b.gguf", gpu_layers=-1)
        >>> model.warmup()
        >>> 
        >>> # With context manager
        >>> with Model.from_local("models/qwen-0.5b.gguf") as model:
        ...     info = model.get_info()
        ...     print(f"Backend: {info['backend']}")
        >>> 
        >>> # Remote model
        >>> model = Model.from_remote(
        ...     endpoint="https://api.openai.com/v1",
        ...     api_key="sk-..."
        ... )
    """
    
    def __init__(
        self,
        path: str | Path,
        *,
        gpu_layers: int = -1,
        context_size: int = 2048,
        threads: int = 0,
        api_key: Optional[str] = None,
        api_base_url: Optional[str] = None,
        backend: Literal["local", "remote"] = "local",
    ):
        """
        Create a new model.
        
        Args:
            path: Path to GGUF model file (local) or API endpoint URL (remote)
            gpu_layers: Number of layers to offload to GPU. 
                       -1 = auto (use all available), 0 = CPU only, N = specific count
            context_size: Context window size in tokens (default: 2048)
            threads: Number of CPU threads (0 = auto-detect based on CPU cores)
            api_key: API key for remote models (optional)
            api_base_url: Custom API endpoint for remote models (optional)
            backend: Backend type - "local" or "remote"
            
        Raises:
            ModelNotFoundError: If model file doesn't exist (local models)
            BackendInitError: If backend initialization fails
            InvalidParameterError: If parameters are invalid
        """
        self._handle: Optional[int] = None
        self._closed = False
        
        # Convert path to string and encode
        path_str = str(path)
        path_bytes = path_str.encode('utf-8')
        api_key_bytes = api_key.encode('utf-8') if api_key else None
        api_base_url_bytes = api_base_url.encode('utf-8') if api_base_url else None
        
        # Create C config structure
        config = _native.CModelConfig(
            path=path_bytes,
            gpu_layers=gpu_layers,
            context_size=context_size,
            threads=threads,
            api_key=api_key_bytes,
            api_base_url=api_base_url_bytes,
        )
        
        # Create model using appropriate backend
        if backend == "local":
            self._handle = _native._lib.luup_model_create_local(config)
        else:  # remote
            self._handle = _native._lib.luup_model_create_remote(config)
        
        if not self._handle:
            error_msg = _native._lib.luup_get_last_error()
            if error_msg:
                msg = error_msg.decode('utf-8')
            else:
                msg = "Failed to create model"
            
            # Check if it's a file not found error for local models
            if backend == "local" and not Path(path).exists():
                raise ModelNotFoundError(f"Model file not found: {path}")
            else:
                raise BackendInitError(msg)
        
        # Register cleanup
        atexit.register(self.close)
    
    @classmethod
    def from_local(
        cls,
        path: str | Path,
        *,
        gpu_layers: int = -1,
        context_size: int = 2048,
        threads: int = 0,
    ) -> Self:
        """
        Create a model from a local GGUF file using llama.cpp backend.
        
        Args:
            path: Path to GGUF model file
            gpu_layers: GPU layers (-1 for auto, 0 for CPU only)
            context_size: Context window size in tokens
            threads: CPU threads (0 for auto)
            
        Returns:
            Model instance
            
        Raises:
            ModelNotFoundError: If model file doesn't exist
            BackendInitError: If llama.cpp initialization fails
        """
        return cls(
            path=path,
            gpu_layers=gpu_layers,
            context_size=context_size,
            threads=threads,
            backend="local",
        )
    
    @classmethod
    def from_remote(
        cls,
        endpoint: str,
        api_key: str,
        *,
        context_size: int = 2048,
    ) -> Self:
        """
        Create a model using a remote OpenAI-compatible API.
        
        Args:
            endpoint: API endpoint URL (e.g., "https://api.openai.com/v1")
            api_key: API key for authentication
            context_size: Context window size in tokens
            
        Returns:
            Model instance
            
        Raises:
            BackendInitError: If API initialization fails
        """
        return cls(
            path=endpoint,
            api_key=api_key,
            context_size=context_size,
            backend="remote",
        )
    
    def warmup(self) -> None:
        """
        Pre-warm the model by running a dummy inference.
        
        This reduces first-token latency for subsequent generations.
        Recommended to call after model creation for better UX.
        
        Raises:
            InferenceError: If warmup fails
        """
        self._check_closed()
        error_code = _native._lib.luup_model_warmup(self._handle)
        check_error(error_code, _native._lib.luup_get_last_error)
    
    def get_info(self) -> Dict[str, Any]:
        """
        Get model information including backend, device, and memory usage.
        
        Returns:
            Dictionary with model information:
                - backend: Backend type (e.g., "llama.cpp", "openai")
                - device: Device used (e.g., "Metal", "CUDA", "CPU")
                - gpu_layers_loaded: Number of layers actually loaded on GPU
                - memory_usage: Estimated memory usage in bytes
                - context_size: Configured context window size
                
        Raises:
            InferenceError: If getting info fails
        """
        self._check_closed()
        info = _native.CModelInfo()
        error_code = _native._lib.luup_model_get_info(self._handle, info)
        check_error(error_code, _native._lib.luup_get_last_error)
        
        return {
            "backend": info.backend.decode('utf-8') if info.backend else "unknown",
            "device": info.device.decode('utf-8') if info.device else "unknown",
            "gpu_layers_loaded": info.gpu_layers_loaded,
            "memory_usage": info.memory_usage,
            "context_size": info.context_size,
        }
    
    def close(self) -> None:
        """
        Explicitly close and free model resources.
        
        Called automatically by context manager and destructor.
        Safe to call multiple times.
        """
        if not self._closed and self._handle:
            _native._lib.luup_model_destroy(self._handle)
            self._handle = None
            self._closed = True
    
    def _check_closed(self) -> None:
        """Raise error if model is closed."""
        if self._closed or not self._handle:
            raise ValueError("Model is closed")
    
    # Context manager protocol
    def __enter__(self) -> Self:
        """Enter context manager."""
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb) -> None:
        """Exit context manager and cleanup."""
        self.close()
    
    # Destructor
    def __del__(self) -> None:
        """Cleanup on garbage collection."""
        self.close()
    
    def __repr__(self) -> str:
        """String representation."""
        status = "closed" if self._closed else "open"
        return f"<Model status={status} handle={self._handle}>"

