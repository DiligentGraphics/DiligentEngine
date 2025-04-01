import sys
import os
import re

def compute_page_id(input_filepath):
    # Determine the directory of this script.
    script_dir = os.path.dirname(os.path.abspath(__file__))
    # The project root is one level up.
    root_dir = os.path.dirname(script_dir)
    # Compute the file's absolute path and then its relative path from the root directory.
    abs_input_path = os.path.abspath(input_filepath)
    rel_path = os.path.relpath(abs_input_path, root_dir)
    # Remove the file extension.
    rel_path_no_ext, _ = os.path.splitext(rel_path)
    # Replace path separators with underscores.
    page_id = rel_path_no_ext.replace(os.sep, "_")
    return page_id

def replace_special_symbols(text):
    # Define your symbol mappings using Unicode escape sequences.
    replacements = {
        ":arrow_forward:": "\u25B6",
        ":heavy_check_mark:": "\u2714",
    }
    for key, value in replacements.items():
        text = text.replace(key, value)
    return text

def process_content(input_filepath, lines):
    page_id = compute_page_id(input_filepath)
    output_lines = []
    header_regex = re.compile(r'^\s*(#+)\s*(.+?)\s*$')
    header_replaced = False

    for line in lines:
        # Apply symbol replacement for every line.
        line = replace_special_symbols(line)

        # Look for the first non-empty line that starts with a Markdown header.
        if not header_replaced and line.strip():
            match = header_regex.match(line)
            if match:
                header_title = match.group(2)
                # Replace the header with the \page command.
                output_lines.append(f"\\page {page_id} {header_title}\n")
                header_replaced = True
                continue
        output_lines.append(line)
    return output_lines

def main():
    if len(sys.argv) < 2:
        sys.stderr.write("Usage: {} <input_file_path>\n".format(sys.argv[0]))
        sys.exit(1)

    input_filepath = sys.argv[1]

    # Open and read the file using the provided file path.
    try:
        with open(input_filepath, "r", encoding="utf-8") as f:
            content = f.readlines()
    except Exception as e:
        sys.stderr.write(f"Error reading {input_filepath}: {e}\n")
        sys.exit(1)

    filtered_content = process_content(input_filepath, content)
    # Write the filtered content to standard output.
    sys.stdout.reconfigure(encoding='utf-8')
    sys.stdout.writelines(filtered_content)

if __name__ == "__main__":
    main()
