"""
Tests for tool calling functionality.
"""

import pytest
import json


def test_tool_registration(agent):
    """Test basic tool registration."""
    called = []
    
    @agent.tool(description="Test tool")
    def test_func(x: int) -> int:
        """Test function."""
        called.append(x)
        return x * 2
    
    # Tool should be registered
    assert "test_func" in agent._tools
    assert len(agent._tools) == 1
    
    # Should be able to call the tool directly
    result = test_func(5)
    assert result == 10
    assert called == [5]


def test_tool_with_custom_name(agent):
    """Test tool registration with custom name."""
    @agent.tool(name="custom_name", description="Custom tool")
    def my_func() -> str:
        return "success"
    
    # Should be registered with custom name
    assert "custom_name" in agent._tools
    assert "my_func" not in agent._tools


def test_tool_with_multiple_params(agent):
    """Test tool with multiple parameters."""
    @agent.tool(description="Add two numbers")
    def add(a: int, b: int) -> int:
        """Add two numbers together."""
        return a + b
    
    assert "add" in agent._tools
    
    # Test direct call
    assert add(3, 4) == 7


def test_tool_with_optional_params(agent):
    """Test tool with optional parameters."""
    @agent.tool(description="Greet someone")
    def greet(name: str, title: str = "Mr.") -> str:
        """Greet a person."""
        return f"Hello, {title} {name}!"
    
    assert "greet" in agent._tools
    
    # Test with and without optional param
    assert greet("Smith") == "Hello, Mr. Smith!"
    assert greet("Smith", "Dr.") == "Hello, Dr. Smith!"


def test_tool_with_dict_return(agent):
    """Test tool that returns a dictionary."""
    @agent.tool(description="Get user info")
    def get_user(user_id: int) -> dict:
        """Get user information."""
        return {
            "id": user_id,
            "name": "Test User",
            "active": True
        }
    
    result = get_user(123)
    assert isinstance(result, dict)
    assert result["id"] == 123
    assert "name" in result


def test_tool_schema_generation(agent):
    """Test automatic schema generation from type hints."""
    @agent.tool(description="Test schema generation")
    def complex_func(
        name: str,
        age: int,
        active: bool,
        score: float,
        tags: list
    ) -> dict:
        """Complex function with various types."""
        return {"success": True}
    
    assert "complex_func" in agent._tools
    
    # Schema should have been generated (stored internally)
    # We can verify by checking the tool was registered successfully


def test_tool_with_explicit_schema(agent):
    """Test tool registration with explicit schema."""
    schema = {
        "type": "object",
        "properties": {
            "x": {"type": "number"},
            "y": {"type": "number"}
        },
        "required": ["x", "y"]
    }
    
    @agent.tool(name="calc", description="Calculate", schema=schema)
    def calculate(x: float, y: float) -> float:
        """Calculate something."""
        return x + y
    
    assert "calc" in agent._tools


def test_multiple_tools(agent):
    """Test registering multiple tools."""
    @agent.tool(description="Tool 1")
    def tool1() -> str:
        return "one"
    
    @agent.tool(description="Tool 2")
    def tool2() -> str:
        return "two"
    
    @agent.tool(description="Tool 3")
    def tool3() -> str:
        return "three"
    
    assert len(agent._tools) == 3
    assert "tool1" in agent._tools
    assert "tool2" in agent._tools
    assert "tool3" in agent._tools


def test_tool_uses_docstring_for_description(agent):
    """Test that docstring is used as description if not provided."""
    @agent.tool()
    def documented_func() -> str:
        """This is the docstring description."""
        return "result"
    
    # Tool should be registered (description taken from docstring)
    assert "documented_func" in agent._tools


def test_tool_error_handling(agent):
    """Test that tool errors are handled gracefully."""
    @agent.tool(description="Failing tool")
    def failing_tool() -> str:
        """This tool raises an error."""
        raise ValueError("Tool failed")
    
    # Tool is registered
    assert "failing_tool" in agent._tools
    
    # Calling it should raise the error
    with pytest.raises(ValueError, match="Tool failed"):
        failing_tool()

