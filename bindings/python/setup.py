"""
Setup script for luup-agent Python package.
"""

from setuptools import setup, find_packages
from pathlib import Path

# Read README
readme_file = Path(__file__).parent / "README.md"
long_description = ""
if readme_file.exists():
    long_description = readme_file.read_text(encoding="utf-8")

setup(
    name="luup-agent",
    version="0.1.0",
    description="Multi-agent LLM library with tool calling",
    long_description=long_description,
    long_description_content_type="text/markdown",
    author="luup-agent contributors",
    author_email="",
    url="https://github.com/gialup03/luup-agent",
    packages=find_packages(),
    package_data={
        "luup_agent": [
            "*.dylib",  # macOS
            "*.so",     # Linux
            "*.dll",    # Windows
        ],
    },
    python_requires=">=3.8",
    install_requires=[
        "typing-extensions>=4.0.0; python_version<'3.10'",
    ],
    extras_require={
        "dev": [
            "pytest>=7.0.0",
            "pytest-asyncio>=0.21.0",
            "pytest-cov>=4.0.0",
            "mypy>=1.0.0",
            "black>=23.0.0",
            "ruff>=0.1.0",
        ],
    },
    classifiers=[
        "Development Status :: 3 - Alpha",
        "Intended Audience :: Developers",
        "Intended Audience :: Science/Research",
        "License :: OSI Approved :: MIT License",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Programming Language :: Python :: 3.12",
        "Programming Language :: C",
        "Topic :: Scientific/Engineering :: Artificial Intelligence",
        "Topic :: Software Development :: Libraries :: Python Modules",
    ],
    keywords="llm ai ml agent tool-calling llama inference",
    project_urls={
        "Bug Reports": "https://github.com/gialup03/luup-agent/issues",
        "Source": "https://github.com/gialup03/luup-agent",
        "Documentation": "https://github.com/gialup03/luup-agent/blob/main/README.md",
    },
)

