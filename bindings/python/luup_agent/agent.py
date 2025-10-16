"""
Agent class for conversational AI with tool calling.

Provides a Pythonic wrapper around the C agent API with support for
tool decorators, streaming, and async generation.
"""

import asyncio
import json
import ctypes
import inspect
from functools import wraps
from typing import (
    Callable, Optional, Iterator, AsyncIterator, Dict, Any, List
)

# For Self type (Python 3.11+)
try:
    from typing import Self
except ImportError:
    from typing_extensions import Self

from . import _native
from .exceptions import check_error
from .model import Model


class Agent:
    """
    AI agent with tool calling support and conversation management.
    
    The agent maintains conversation history and can automatically call
    registered tools during generation.
    
    Examples:
        >>> # Basic usage
        >>> model = Model.from_local("models/qwen-0.5b.gguf")
        >>> agent = Agent(model, system_prompt="You are a helpful assistant.")
        >>> response = agent.generate("Hello!")
        >>> print(response)
        
        >>> # With tool calling
        >>> @agent.tool(description="Get weather for a city")
        >>> def get_weather(city: str) -> dict:
        ...     return {"temperature": 72, "condition": "sunny"}
        >>> 
        >>> response = agent.generate("What's the weather in Paris?")
        
        >>> # Streaming
        >>> for token in agent.generate_stream("Tell me a story"):
        ...     print(token, end='', flush=True)
        
        >>> # Async streaming
        >>> async for token in agent.generate_async("Hello"):
        ...     print(token, end='', flush=True)
    """
    
    def __init__(
        self,
        model: Model,
        *,
        system_prompt: str = "",
        temperature: float = 0.7,
        max_tokens: int = 512,
        enable_tool_calling: bool = True,
        enable_history: bool = True,
        enable_builtin_tools: bool = True,
    ):
        """
        Create a new agent.
        
        Args:
            model: Model to use for generation (can be shared across agents)
            system_prompt: System prompt defining agent behavior and role
            temperature: Sampling temperature (0.0 = deterministic, 2.0 = very random)
            max_tokens: Maximum tokens to generate per response (0 = no limit)
            enable_tool_calling: Enable automatic tool calling
            enable_history: Enable automatic conversation history management
            enable_builtin_tools: Enable built-in tools (todo, notes, summarization) - opt-out design
            
        Raises:
            InvalidParameterError: If parameters are invalid
        """
        self._handle: Optional[int] = None
        self._model = model
        self._closed = False
        self._tools: Dict[str, Callable] = {}
        self._tool_callbacks: Dict[str, _native.CToolCallback] = {}
        
        # Create C config structure
        config = _native.CAgentConfig(
            model=model._handle,
            system_prompt=system_prompt.encode('utf-8'),
            temperature=temperature,
            max_tokens=max_tokens,
            enable_tool_calling=enable_tool_calling,
            enable_history_management=enable_history,
            enable_builtin_tools=enable_builtin_tools,
        )
        
        # Create agent
        self._handle = _native._lib.luup_agent_create(config)
        if not self._handle:
            error_msg = _native._lib.luup_get_last_error()
            msg = error_msg.decode('utf-8') if error_msg else "Failed to create agent"
            raise RuntimeError(msg)
    
    def generate(self, message: str) -> str:
        """
        Generate a complete response (blocking).
        
        This method blocks until the full response is generated. For streaming
        output, use generate_stream() or generate_async() instead.
        
        Args:
            message: User message to respond to
            
        Returns:
            Generated response text
            
        Raises:
            InferenceError: If generation fails
            ToolNotFoundError: If agent tries to call unregistered tool
        """
        self._check_closed()
        
        # Call C function
        result_ptr = _native._lib.luup_agent_generate(
            self._handle,
            message.encode('utf-8')
        )
        
        if not result_ptr:
            error_msg = _native._lib.luup_get_last_error()
            msg = error_msg.decode('utf-8') if error_msg else "Generation failed"
            raise RuntimeError(msg)
        
        # Decode response
        response = result_ptr.decode('utf-8')
        
        # Free C string
        _native._lib.luup_free_string(result_ptr)
        
        return response
    
    def generate_stream(self, message: str) -> Iterator[str]:
        """
        Generate response with token-by-token streaming (blocking iterator).
        
        Yields tokens as they're generated. This is a synchronous iterator
        that blocks on each token.
        
        Args:
            message: User message to respond to
            
        Yields:
            Generated tokens as strings
            
        Example:
            >>> for token in agent.generate_stream("Hello"):
            ...     print(token, end='', flush=True)
            
        Raises:
            InferenceError: If generation fails
        """
        self._check_closed()
        
        tokens: List[str] = []
        
        # Create callback that collects tokens
        @_native.CStreamCallback
        def callback(token_ptr, user_data):
            if token_ptr:
                token = token_ptr.decode('utf-8')
                tokens.append(token)
        
        # Call C function
        error_code = _native._lib.luup_agent_generate_stream(
            self._handle,
            message.encode('utf-8'),
            callback,
            None
        )
        
        check_error(error_code, _native._lib.luup_get_last_error)
        
        # Yield collected tokens
        yield from tokens
    
    async def generate_async(self, message: str) -> AsyncIterator[str]:
        """
        Generate response asynchronously with streaming.
        
        This is an async generator that yields tokens without blocking the
        event loop. Uses asyncio.to_thread() to run blocking operations.
        
        Args:
            message: User message to respond to
            
        Yields:
            Generated tokens as strings
            
        Example:
            >>> async for token in agent.generate_async("Hello"):
            ...     print(token, end='', flush=True)
            
        Raises:
            InferenceError: If generation fails
        """
        # Run streaming in a thread to not block event loop
        loop = asyncio.get_event_loop()
        
        # Use a queue to pass tokens from thread to async context
        queue: asyncio.Queue[Optional[str]] = asyncio.Queue()
        
        def run_stream():
            """Run streaming in thread and put tokens in queue."""
            try:
                for token in self.generate_stream(message):
                    # Use call_soon_threadsafe to put in queue from thread
                    loop.call_soon_threadsafe(queue.put_nowait, token)
            except Exception as e:
                loop.call_soon_threadsafe(queue.put_nowait, e)
            finally:
                # Signal completion
                loop.call_soon_threadsafe(queue.put_nowait, None)
        
        # Start streaming in thread
        await asyncio.to_thread(run_stream)
        
        # Yield tokens from queue
        while True:
            token = await queue.get()
            if token is None:
                break
            if isinstance(token, Exception):
                raise token
            yield token
    
    def tool(
        self,
        name: Optional[str] = None,
        description: Optional[str] = None,
        schema: Optional[Dict[str, Any]] = None,
    ) -> Callable:
        """
        Decorator to register a function as a tool.
        
        The tool can be called by the agent during generation when appropriate.
        Parameter schema can be provided explicitly or auto-generated from
        type hints.
        
        Args:
            name: Tool name (defaults to function name)
            description: Tool description (defaults to docstring first line)
            schema: JSON schema for parameters (auto-generated from type hints if None)
            
        Returns:
            Decorator function
            
        Example:
            >>> @agent.tool(description="Get weather for a city")
            >>> def get_weather(city: str, units: str = "celsius") -> dict:
            ...     '''Get current weather.'''
            ...     return {"temperature": 72, "condition": "sunny"}
            
            >>> # With explicit schema
            >>> @agent.tool(
            ...     name="calculate",
            ...     schema={
            ...         "type": "object",
            ...         "properties": {
            ...             "expression": {"type": "string"}
            ...         },
            ...         "required": ["expression"]
            ...     }
            ... )
            >>> def calc(expression: str) -> float:
            ...     return eval(expression)
        """
        def decorator(func: Callable) -> Callable:
            # Determine tool name
            tool_name = name or func.__name__
            
            # Determine description
            tool_desc = description
            if tool_desc is None and func.__doc__:
                # Use first line of docstring
                tool_desc = func.__doc__.strip().split('\n')[0]
            if not tool_desc:
                tool_desc = f"Tool: {tool_name}"
            
            # Generate schema if not provided
            tool_schema = schema
            if tool_schema is None:
                tool_schema = self._generate_schema_from_function(func)
            
            # Register tool
            self._register_tool(tool_name, tool_desc, tool_schema, func)
            
            return func
        
        return decorator
    
    def _generate_schema_from_function(self, func: Callable) -> Dict[str, Any]:
        """
        Auto-generate JSON schema from function signature and type hints.
        
        Args:
            func: Function to analyze
            
        Returns:
            JSON schema dictionary
        """
        sig = inspect.signature(func)
        properties = {}
        required = []
        
        for param_name, param in sig.parameters.items():
            if param_name == 'self':
                continue
            
            # Determine type
            param_type = "string"  # default
            if param.annotation != inspect.Parameter.empty:
                ann = param.annotation
                if ann == str:
                    param_type = "string"
                elif ann in (int, float):
                    param_type = "number"
                elif ann == bool:
                    param_type = "boolean"
                elif ann in (list, List):
                    param_type = "array"
                elif ann in (dict, Dict):
                    param_type = "object"
            
            properties[param_name] = {"type": param_type}
            
            # Check if required (no default value)
            if param.default == inspect.Parameter.empty:
                required.append(param_name)
        
        schema = {
            "type": "object",
            "properties": properties,
        }
        
        if required:
            schema["required"] = required
        
        return schema
    
    def _register_tool(
        self,
        name: str,
        description: str,
        schema: Dict[str, Any],
        func: Callable
    ) -> None:
        """
        Internal method to register a tool with the C API.
        
        Args:
            name: Tool name
            description: Tool description
            schema: JSON schema for parameters
            func: Python function to call
        """
        self._check_closed()
        
        # Store function
        self._tools[name] = func
        
        # Create C callback that wraps Python function
        @_native.CToolCallback
        def callback(params_json_ptr, user_data):
            try:
                # Decode parameters
                params_json = params_json_ptr.decode('utf-8') if params_json_ptr else "{}"
                params = json.loads(params_json)
                
                # Call Python function
                result = func(**params)
                
                # Convert result to JSON
                result_json = json.dumps(result)
                
                # Return as C string (will be freed by C code)
                return result_json.encode('utf-8')
            
            except Exception as e:
                # Return error as JSON
                error_json = json.dumps({"error": str(e)})
                return error_json.encode('utf-8')
        
        # Store callback to prevent garbage collection
        self._tool_callbacks[name] = callback
        
        # Create C tool structure
        schema_json = json.dumps(schema)
        tool = _native.CTool(
            name=name.encode('utf-8'),
            description=description.encode('utf-8'),
            parameters_json=schema_json.encode('utf-8'),
        )
        
        # Register with C API
        error_code = _native._lib.luup_agent_register_tool(
            self._handle,
            tool,
            callback,
            None
        )
        
        check_error(error_code, _native._lib.luup_get_last_error)
    
    def add_message(self, role: str, content: str) -> None:
        """
        Manually add a message to conversation history.
        
        Args:
            role: Message role - "user", "assistant", or "system"
            content: Message content
            
        Raises:
            InvalidParameterError: If role is invalid
        """
        self._check_closed()
        error_code = _native._lib.luup_agent_add_message(
            self._handle,
            role.encode('utf-8'),
            content.encode('utf-8')
        )
        check_error(error_code, _native._lib.luup_get_last_error)
    
    def clear_history(self) -> None:
        """
        Clear conversation history.
        
        This removes all messages except the system prompt.
        """
        self._check_closed()
        error_code = _native._lib.luup_agent_clear_history(self._handle)
        check_error(error_code, _native._lib.luup_get_last_error)
    
    def get_history(self) -> List[Dict[str, str]]:
        """
        Get conversation history as list of message dictionaries.
        
        Returns:
            List of message dicts with "role" and "content" keys
            
        Example:
            >>> history = agent.get_history()
            >>> for msg in history:
            ...     print(f"{msg['role']}: {msg['content']}")
        """
        self._check_closed()
        
        # Get JSON from C
        json_ptr = _native._lib.luup_agent_get_history_json(self._handle)
        if not json_ptr:
            return []
        
        # Decode and parse
        json_str = json_ptr.decode('utf-8')
        _native._lib.luup_free_string(json_ptr)
        
        try:
            history = json.loads(json_str)
            return history if isinstance(history, list) else []
        except json.JSONDecodeError:
            return []
    
    def enable_builtin_todo(self, storage_path: Optional[str] = None) -> None:
        """
        Enable built-in todo list tool.
        
        Args:
            storage_path: Path to store todo list JSON (None for memory only)
        """
        self._check_closed()
        path_bytes = storage_path.encode('utf-8') if storage_path else None
        error_code = _native._lib.luup_agent_enable_builtin_todo(
            self._handle,
            path_bytes
        )
        check_error(error_code, _native._lib.luup_get_last_error)
    
    def enable_builtin_notes(self, storage_path: Optional[str] = None) -> None:
        """
        Enable built-in notes tool.
        
        Args:
            storage_path: Path to store notes JSON (None for memory only)
        """
        self._check_closed()
        path_bytes = storage_path.encode('utf-8') if storage_path else None
        error_code = _native._lib.luup_agent_enable_builtin_notes(
            self._handle,
            path_bytes
        )
        check_error(error_code, _native._lib.luup_get_last_error)
    
    def enable_builtin_summarization(self) -> None:
        """
        Enable built-in auto-summarization.
        
        Automatically summarizes conversation history when context fills up.
        """
        self._check_closed()
        error_code = _native._lib.luup_agent_enable_builtin_summarization(
            self._handle
        )
        check_error(error_code, _native._lib.luup_get_last_error)
    
    def close(self) -> None:
        """
        Explicitly close and free agent resources.
        
        Called automatically by context manager and destructor.
        Safe to call multiple times.
        """
        if not self._closed and self._handle:
            _native._lib.luup_agent_destroy(self._handle)
            self._handle = None
            self._closed = True
    
    def _check_closed(self) -> None:
        """Raise error if agent is closed."""
        if self._closed or not self._handle:
            raise ValueError("Agent is closed")
    
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
        num_tools = len(self._tools)
        return f"<Agent status={status} tools={num_tools}>"

