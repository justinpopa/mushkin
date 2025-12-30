#!/usr/bin/env python3
"""
Lua API Documentation Generator

Parses C++ source files to extract Lua API documentation and generates
markdown files suitable for GitHub wiki.

Usage:
    python scripts/generate-lua-docs.py [--output-dir docs/wiki]
    python scripts/generate-lua-docs.py --version 0.1.0
    python scripts/generate-lua-docs.py --version latest
"""

import os
import re
import argparse
import subprocess
from pathlib import Path
from dataclasses import dataclass, field
from typing import Optional
from datetime import datetime


@dataclass
class LuaFunction:
    """Represents a documented Lua API function."""
    name: str
    signature: str = ""
    description: str = ""
    params: list = field(default_factory=list)      # List of (name, type, desc)
    returns: list = field(default_factory=list)     # List of (type, desc)
    examples: list = field(default_factory=list)    # List of code examples
    see_also: list = field(default_factory=list)    # List of related function names
    category: str = ""
    source_file: str = ""
    line_number: int = 0


@dataclass
class Category:
    """A category of Lua functions."""
    name: str
    title: str
    description: str = ""
    functions: list = field(default_factory=list)


# Map source files to categories
FILE_CATEGORIES = {
    "world_output.cpp": ("Output", "Output Functions", "Functions for displaying text in the output window."),
    "world_network.cpp": ("Network", "Network Functions", "Functions for sending data to the MUD server."),
    "world_variables.cpp": ("Variables", "Variable Functions", "Functions for managing world variables."),
    "world_colors.cpp": ("Colors", "Color Functions", "Functions for color conversion and manipulation."),
    "world_logging.cpp": ("Logging", "Logging Functions", "Functions for log file management."),
    "world_aliases.cpp": ("Aliases", "Alias Functions", "Functions for creating and managing aliases."),
    "world_triggers.cpp": ("Triggers", "Trigger Functions", "Functions for creating and managing triggers."),
    "world_timers.cpp": ("Timers", "Timer Functions", "Functions for creating and managing timers."),
    "world_miniwindows.cpp": ("MiniWindows", "MiniWindow Functions", "Functions for creating and drawing miniwindows."),
    "world_arrays.cpp": ("Arrays", "Array Functions", "Functions for managing named arrays (deprecated, use Lua tables)."),
    "world_info.cpp": ("Info", "Information Functions", "Functions for getting world and connection information."),
    "world_utilities.cpp": ("Utilities", "Utility Functions", "General utility functions for string manipulation, encoding, etc."),
    "world_commands.cpp": ("Commands", "Command Functions", "Functions for command queue management."),
    "world_speedwalk.cpp": ("Speedwalk", "Speedwalk Functions", "Functions for speedwalk/pathfinding."),
    "world_database.cpp": ("Database", "Database Functions", "Functions for SQLite database operations."),
    "world_plugins.cpp": ("Plugins", "Plugin Functions", "Functions for inter-plugin communication and plugin management."),
    "lua_utils.cpp": ("Utils", "Utils Library", "The utils.* library functions."),
}


def parse_docstring(comment: str) -> dict:
    """
    Parse a docstring comment to extract all documentation elements.

    Returns dict with: signature, description, params, returns, examples, see_also
    """
    lines = comment.strip().split('\n')
    result = {
        'signature': '',
        'description': [],
        'params': [],
        'returns': [],
        'examples': [],
        'see_also': []
    }

    current_example = []
    in_example = False

    for i, line in enumerate(lines):
        # Remove comment markers and leading/trailing whitespace
        line = re.sub(r'^\s*\*\s?', '', line)
        line = re.sub(r'^/\*\*\s*', '', line)
        line = re.sub(r'\s*\*/$', '', line)

        # First non-empty line with world. or utils. is the signature
        if not result['signature'] and ('world.' in line or 'utils.' in line):
            result['signature'] = line.strip()
            continue

        # Parse @param - format: @param name (type) description
        param_match = re.match(r'@param\s+(\w+)\s+\(([^)]+)\)\s*(.*)', line)
        if param_match:
            in_example = False
            result['params'].append({
                'name': param_match.group(1),
                'type': param_match.group(2),
                'desc': param_match.group(3).strip()
            })
            continue

        # Parse @return - format: @return (type) description
        return_match = re.match(r'@return\s+\(([^)]+)\)\s*(.*)', line)
        if return_match:
            in_example = False
            result['returns'].append({
                'type': return_match.group(1),
                'desc': return_match.group(2).strip()
            })
            continue

        # Parse @see - format: @see Func1, Func2, Func3
        see_match = re.match(r'@see\s+(.*)', line)
        if see_match:
            in_example = False
            funcs = [f.strip() for f in see_match.group(1).split(',')]
            result['see_also'].extend(funcs)
            continue

        # Parse @example - starts a code block
        if line.strip() == '@example':
            if current_example:
                result['examples'].append('\n'.join(current_example))
            current_example = []
            in_example = True
            continue

        # If in example, collect code lines
        if in_example:
            if line.startswith('@') or (not line and not current_example):
                if current_example:
                    result['examples'].append('\n'.join(current_example))
                    current_example = []
                in_example = False
            else:
                current_example.append(line)
                continue

        # Skip empty lines at the start
        if not line.strip() and not result['description']:
            continue

        # Skip lines that are just tag continuations (indented descriptions)
        if line.startswith('  ') and (result['params'] or result['returns']):
            # Append to last param/return description
            if result['returns'] and not line.startswith('@'):
                result['returns'][-1]['desc'] += ' ' + line.strip()
            elif result['params'] and not line.startswith('@'):
                result['params'][-1]['desc'] += ' ' + line.strip()
            continue

        # Regular description line
        if not line.startswith('@'):
            result['description'].append(line.strip())

    # Capture final example if any
    if current_example:
        result['examples'].append('\n'.join(current_example))

    result['description'] = '\n'.join(result['description']).strip()
    return result


def extract_functions_from_file(filepath: Path) -> list[LuaFunction]:
    """Extract Lua function documentation from a C++ source file."""
    functions = []

    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()

    # Pattern to match docstring followed by function definition
    # Matches: /** ... */ int L_FunctionName(lua_State* L)
    # Use [^*] or *[^/] to avoid matching across multiple docstrings
    pattern = r'/\*\*((?:[^*]|\*(?!/))*)\*/\s*\n\s*int\s+(L_\w+)\s*\(\s*lua_State'

    for match in re.finditer(pattern, content, re.DOTALL):
        docstring = match.group(1)
        c_func_name = match.group(2)

        # Get line number
        line_number = content[:match.start()].count('\n') + 1

        parsed = parse_docstring(docstring)

        # Extract Lua function name from signature or C function name
        signature = parsed['signature']
        if signature:
            # Try to extract name from signature like "world.Note(text, ...)"
            name_match = re.match(r'(?:world|utils)\.(\w+)', signature)
            if name_match:
                lua_name = name_match.group(1)
            else:
                # Fall back to C function name without L_ prefix
                lua_name = c_func_name[2:] if c_func_name.startswith('L_') else c_func_name
        else:
            lua_name = c_func_name[2:] if c_func_name.startswith('L_') else c_func_name
            signature = f"world.{lua_name}(...)"

        func = LuaFunction(
            name=lua_name,
            signature=signature,
            description=parsed['description'],
            params=parsed['params'],
            returns=parsed['returns'],
            examples=parsed['examples'],
            see_also=parsed['see_also'],
            source_file=filepath.name,
            line_number=line_number
        )
        functions.append(func)

    return functions


def generate_category_page(category: Category) -> str:
    """Generate markdown content for a category page."""
    lines = [
        f"# {category.title}",
        "",
        category.description,
        "",
        "## Functions",
        "",
    ]

    # Table of contents
    for func in sorted(category.functions, key=lambda f: f.name.lower()):
        lines.append(f"- [{func.name}](#{func.name.lower()})")

    lines.append("")
    lines.append("---")
    lines.append("")

    # Function documentation
    for func in sorted(category.functions, key=lambda f: f.name.lower()):
        lines.extend([
            f"## {func.name}",
            "",
            "```lua",
            func.signature,
            "```",
            "",
        ])

        if func.description:
            lines.append(func.description)
            lines.append("")

        # Parameters
        if func.params:
            lines.append("### Parameters")
            lines.append("")
            lines.append("| Name | Type | Description |")
            lines.append("|------|------|-------------|")
            for param in func.params:
                desc = param['desc'].replace('|', '\\|')  # Escape pipes
                lines.append(f"| `{param['name']}` | {param['type']} | {desc} |")
            lines.append("")

        # Returns
        if func.returns:
            lines.append("### Returns")
            lines.append("")
            for ret in func.returns:
                lines.append(f"- **{ret['type']}**: {ret['desc']}")
            lines.append("")

        # Examples
        if func.examples:
            lines.append("### Example")
            lines.append("")
            for example in func.examples:
                lines.append("```lua")
                lines.append(example)
                lines.append("```")
                lines.append("")

        # See Also
        if func.see_also:
            lines.append("### See Also")
            lines.append("")
            see_links = [f"[{name}](#{name.lower()})" for name in func.see_also]
            lines.append(", ".join(see_links))
            lines.append("")

        lines.append("---")
        lines.append("")

    return '\n'.join(lines)


def generate_index_page(categories: dict[str, Category], version: str = None) -> str:
    """Generate the main index/sidebar page."""
    version_suffix = f"-{version}" if version else ""
    version_display = f" (v{version})" if version else ""

    lines = [
        f"# Lua API Reference{version_display}",
        "",
        "This documentation is automatically generated from the Mushkin source code.",
        "",
    ]

    if version:
        lines.extend([
            f"**Version:** {version}",
            "",
            "See [[API Versions]] for other versions.",
            "",
        ])

    lines.extend([
        "## Categories",
        "",
    ])

    for cat_id in sorted(categories.keys()):
        cat = categories[cat_id]
        func_count = len(cat.functions)
        lines.append(f"- [[{cat.title}|Lua-API{version_suffix}-{cat_id}]] ({func_count} functions)")

    lines.extend([
        "",
        "---",
        "",
        f"*Generated automatically from source code.*",
    ])

    return '\n'.join(lines)


def generate_sidebar(categories: dict[str, Category], version: str = None, all_versions: list = None) -> str:
    """Generate GitHub wiki sidebar."""
    version_suffix = f"-{version}" if version else ""

    lines = [
        "**Mushkin Wiki**",
        "",
        "- [[Home]]",
        "",
        "**Lua API**",
        "",
        "- [[API Versions]]",
    ]

    if version:
        lines.append(f"- [[v{version} Reference|Lua-API-{version}]]")
        for cat_id in sorted(categories.keys()):
            cat = categories[cat_id]
            lines.append(f"  - [[{cat.title}|Lua-API-{version}-{cat_id}]]")
    else:
        lines.append("- [[Latest Reference|Lua-API]]")
        for cat_id in sorted(categories.keys()):
            cat = categories[cat_id]
            lines.append(f"  - [[{cat.title}|Lua-API-{cat_id}]]")

    return '\n'.join(lines)


def generate_versions_page(versions: list, latest: str = None) -> str:
    """Generate the API versions index page."""
    lines = [
        "# API Versions",
        "",
        "Mushkin maintains documentation for each release. Select a version below:",
        "",
    ]

    if latest:
        lines.extend([
            f"**Latest:** [[v{latest}|Lua-API-{latest}]]",
            "",
            "## All Versions",
            "",
        ])

    for version in sorted(versions, reverse=True):
        lines.append(f"- [[v{version}|Lua-API-{version}]]")

    lines.extend([
        "",
        "---",
        "",
        "*Documentation is generated at each release and preserved for reference.*",
    ])

    return '\n'.join(lines)


def get_project_version(project_root: Path) -> str:
    """Extract version from CMakeLists.txt."""
    cmake_file = project_root / "CMakeLists.txt"
    if cmake_file.exists():
        content = cmake_file.read_text()
        match = re.search(r'project\s*\([^)]*VERSION\s+([0-9.]+)', content)
        if match:
            return match.group(1)
    return "0.0.0"


def main():
    parser = argparse.ArgumentParser(description='Generate Lua API documentation')
    parser.add_argument('--source-dir', default='src/world/lua_api',
                        help='Source directory containing Lua API C++ files')
    parser.add_argument('--output-dir', default='docs/wiki',
                        help='Output directory for generated markdown')
    parser.add_argument('--utils-file', default='src/world/lua_utils.cpp',
                        help='Path to lua_utils.cpp for utils.* functions')
    parser.add_argument('--version', default=None,
                        help='Version tag for docs (e.g., 0.1.0). If not specified, uses project version.')
    parser.add_argument('--update-versions-page', action='store_true',
                        help='Update the API-Versions.md page with this version')
    args = parser.parse_args()

    # Find project root
    script_dir = Path(__file__).parent
    project_root = script_dir.parent

    source_dir = project_root / args.source_dir
    output_dir = project_root / args.output_dir
    utils_file = project_root / args.utils_file

    # Determine version
    version = args.version or get_project_version(project_root)
    version_suffix = f"-{version}"

    print(f"Generating documentation for version {version}")

    # Create output directory
    output_dir.mkdir(parents=True, exist_ok=True)

    # Initialize categories
    categories: dict[str, Category] = {}

    # Process each source file
    for filename, (cat_id, title, desc) in FILE_CATEGORIES.items():
        filepath = source_dir / filename
        if not filepath.exists():
            # Try utils file location
            if filename == "lua_utils.cpp":
                filepath = utils_file

        if not filepath.exists():
            print(f"Warning: {filepath} not found, skipping")
            continue

        functions = extract_functions_from_file(filepath)

        if cat_id not in categories:
            categories[cat_id] = Category(name=cat_id, title=title, description=desc)

        for func in functions:
            func.category = cat_id
            categories[cat_id].functions.append(func)

        print(f"Extracted {len(functions)} functions from {filename}")

    # Generate versioned category pages
    for cat_id, category in categories.items():
        if category.functions:
            content = generate_category_page(category)
            # Add version header to content
            content = f"<!-- Version: {version} -->\n" + content
            output_file = output_dir / f"Lua-API{version_suffix}-{cat_id}.md"
            with open(output_file, 'w', encoding='utf-8') as f:
                f.write(content)
            print(f"Generated {output_file}")

    # Generate versioned index page
    index_content = generate_index_page(categories, version)
    index_file = output_dir / f"Lua-API{version_suffix}.md"
    with open(index_file, 'w', encoding='utf-8') as f:
        f.write(index_content)
    print(f"Generated {index_file}")

    # Update versions page if requested
    if args.update_versions_page:
        versions_file = output_dir / "API-Versions.md"
        existing_versions = []

        # Read existing versions from page if it exists
        if versions_file.exists():
            content = versions_file.read_text()
            # Extract versions from links like [[v0.1.0|Lua-API-0.1.0]]
            existing_versions = re.findall(r'\[\[v([0-9.]+)\|Lua-API-', content)

        # Add current version if not already present
        if version not in existing_versions:
            existing_versions.append(version)

        # Generate updated versions page
        versions_content = generate_versions_page(existing_versions, latest=version)
        with open(versions_file, 'w', encoding='utf-8') as f:
            f.write(versions_content)
        print(f"Updated {versions_file}")

        # Generate/update sidebar with latest version
        sidebar_content = generate_sidebar(categories, version, existing_versions)
        sidebar_file = output_dir / "_Sidebar.md"
        with open(sidebar_file, 'w', encoding='utf-8') as f:
            f.write(sidebar_content)
        print(f"Generated {sidebar_file}")

    # Summary
    total_functions = sum(len(c.functions) for c in categories.values())
    print(f"\nTotal: {total_functions} functions documented across {len(categories)} categories")
    print(f"Version: {version}")


if __name__ == '__main__':
    main()
