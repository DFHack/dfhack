import sys
import re

TEMPLATE_SECTIONS = [
    "## New Tools",
    "## New Features",
    "## Fixes",
    "## Misc Improvements",
    "## Documentation",
    "## API",
    "## Lua",
    "## Removed"
]

def chunk_changelog(lines):
    """Splits the changelog into three parts: before, future section, after."""
    before, future_section, after = [], [], []
    current_section = before
    future_found = False

    for line in lines:
        if line.startswith("# Future") and not future_found:
            future_found = True
            current_section = future_section
        elif future_found and re.match(r"^# \d", line):  # Next version starts
            current_section = after

        current_section.append(line)

    return before, future_section, after

def clean_old_future_section(future_section, new_version):
    """Renames Future section and removes empty subsections."""
    cleaned_section = [f"# {new_version}\n"]
    section_start = None
    non_empty_sections = set()

    for i, line in enumerate(future_section[1:]):  # Skip "Future" header
        if re.match(r"^## ", line):  # Track section starts
            section_start = len(cleaned_section)
        elif re.match(r"^- ", line.strip()):  # Found a list entry, mark section as non-empty
            non_empty_sections.add(section_start)

        cleaned_section.append(line)

    # Remove empty sections
    final_section = []
    i = 0
    while i < len(cleaned_section):
        if re.match(r"^## ", cleaned_section[i]) and i in non_empty_sections:
            final_section.append(cleaned_section[i])
        elif re.match(r"^## ", cleaned_section[i]) and i not in non_empty_sections:
            i += 1  # Skip section header
            while i < len(cleaned_section) and not re.match(r"^(## |# )", cleaned_section[i]):
                i += 1  # Skip body of the empty section
            continue  # Avoid double increment
        else:
            final_section.append(cleaned_section[i])
        i += 1

    return final_section

def update_changelog(file_path, new_version):
    with open(file_path, "r", encoding="utf-8") as f:
        lines = f.readlines()

    before, future_section, after = chunk_changelog(lines)

    if not future_section:
        print("No 'Future' section found!")
        return

    cleaned_future_section = clean_old_future_section(future_section, new_version)

    # Build new Future section
    new_future_section = ["# Future\n\n"]
    for section in TEMPLATE_SECTIONS:
        new_future_section.append(section + "\n\n")

    # Reassemble changelog
    updated_changelog = before + new_future_section + cleaned_future_section + after

    # Write back to the file
    with open(file_path, "w", encoding="utf-8") as f:
        f.writelines(updated_changelog)

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python update_changelog.py <changelog_file_path> <new_version>")
        sys.exit(1)

    update_changelog(sys.argv[1], sys.argv[2])
