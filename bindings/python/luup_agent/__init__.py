"""
luup-agent: Multi-Agent LLM Library

A cross-platform Python library for LLM inference with multi-agent support
and tool calling. Powered by llama.cpp for local models and supports
OpenAI-compatible APIs for remote models.

Examples:
    >>> from luup_agent import Model, Agent
    >>> 
    >>> # Create a local model
    >>> model = Model.from_local("models/qwen-0.5b.gguf", gpu_layers=-1)
    >>> 
    >>> # Create an agent
    >>> agent = Agent(model, system_prompt="You are a helpful assistant.")
    >>> 
    >>> # Register a tool
    >>> @agent.tool(description="Get weather for a city")
    >>> def get_weather(city: str) -> dict:
    ...     return {"temperature": 72, "condition": "sunny"}
    >>> 
    >>> # Generate response
    >>> response = agent.generate("What's the weather in Paris?")
    >>> print(response)
"""

from .model import Model
from .agent import Agent
from .exceptions import (
    LuupError,
    InvalidParameterError,
    OutOfMemoryError,
    ModelNotFoundError,
    InferenceError,
    ToolNotFoundError,
    JsonParseError,
    HttpError,
    BackendInitError,
)
from ._native import get_version, get_version_tuple

__version__ = get_version()
__version_info__ = get_version_tuple()

__all__ = [
    # Main classes
    "Model",
    "Agent",
    # Exceptions
    "LuupError",
    "InvalidParameterError",
    "OutOfMemoryError",
    "ModelNotFoundError",
    "InferenceError",
    "ToolNotFoundError",
    "JsonParseError",
    "HttpError",
    "BackendInitError",
    # Version info
    "__version__",
    "__version_info__",
]

