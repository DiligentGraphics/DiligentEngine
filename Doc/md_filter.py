import sys
import os
import re


def compute_page_id(root_dir, input_filepath):
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


def replace_github_blob_urls(text):
    """
    Replaces '/blob/' with '/raw/' in GitHub image URLs.

    For example:
    https://github.com/DiligentGraphics/DiligentSamples/blob/master/...
    becomes:
    https://github.com/DiligentGraphics/DiligentSamples/raw/master/...
    """
    # This regex looks for URLs starting with https://github.com/
    # that have the structure: /<user>/<repo>/blob/<rest>
    pattern = r'(https://github\.com/[^/]+/[^/]+/)(blob)/(.*?)'
    # Replace the matched 'blob' with 'raw'
    return re.sub(pattern, r'\1raw/\3', text)


def replace_relative_image_paths(text, root_dir, input_filepath):
    """
    Replaces relative image paths in Markdown image references with GitHub URLs.

    Markdown references of the form:
      ![Alt Text](relative/image/path)
    
    are replaced as follows:
    1. Compute the full relative path of the image with respect to the project root.
       The project root is defined as one level up from the directory of this script.
    2. If the first-level folder in the computed path starts with "Diligent", treat it as a submodule:
         https://github.com/DiligentGraphics/<Submodule>/raw/master/<rest_of_path>
       Otherwise, assume it belongs to the root module:
         https://github.com/DiligentGraphics/raw/master/<computed_relative_path>
    """
    
    # Regular expression to match Markdown image references.
    pattern = r'!\[([^\]]*)\]\(([^)]+)\)'
    
    def repl(match):
        alt_text = match.group(1)
        image_path = match.group(2)
        # If the path is already absolute (starts with "http"), leave it unchanged.
        if image_path.startswith("http"):
            return match.group(0)
        
        # Compute the full path of the image relative to the project root.
        # The image path is relative to the input file location.
        input_dir = os.path.dirname(os.path.abspath(input_filepath))
        full_image_path = os.path.join(input_dir, image_path)
        # Now compute the relative path from the project root.
        relative_path = os.path.relpath(full_image_path, root_dir)
        # Convert OS-specific path separators to forward slashes.
        relative_path_url = relative_path.replace(os.sep, '/')
        
        # Split the path to check the first folder.
        parts = relative_path_url.split('/')
        if parts and parts[0].startswith("Diligent"):
            module = parts[0]
            rest = '/'.join(parts[1:])
            new_url = f"https://github.com/DiligentGraphics/{module}/raw/master/{rest}"
        else:
            new_url = f"https://github.com/DiligentGraphics/raw/master/{relative_path_url}"
        
        return f"![{alt_text}]({new_url})"
    
    # Replace all Markdown image references in the text.
    return re.sub(pattern, repl, text)


def process_content(input_filepath, lines):
    # Determine the directory of this script.
    script_dir = os.path.dirname(os.path.abspath(__file__))
    # The project root is one level up.
    root_dir = os.path.dirname(script_dir)
    
    page_id = compute_page_id(root_dir, input_filepath)
    output_lines = []
    header_regex = re.compile(r'^\s*(#+)\s*(.+?)\s*$')
    header_replaced = False

    for line in lines:
        # Apply symbol replacement for every line.
        line = replace_special_symbols(line)

        # Fix image URLs.
        line = replace_github_blob_urls(line)
        
        # Replace relative image paths in Markdown image references with GitHub URLs.
        line = replace_relative_image_paths(line, root_dir, input_filepath)
        
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
