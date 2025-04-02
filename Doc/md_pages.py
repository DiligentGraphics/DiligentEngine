import os
import sys
import fnmatch
from md_utils import get_project_root, compute_page_id
#import json

# Directories to exclude explicitly (relative to the project root).
EXCLUDED_DIRS = [
    "Doc",
    "Media",
    "DiligentCore/BuildTools/Android",
    "DiligentSamples/Android",
    "DiligentSamples/Samples/Asteroids/SDK",
    "DiligentFX/Shaders/PostProcess/SuperResolution/private/fsr1",
    "DiligentTools/NativeApp/Android",
    "Tests"
]

# Additional exclusion patterns.
ADDITIONAL_EXCLUDE_PATTERNS = [
    "build/*",
    "*/build/*",
    "*/ThirdParty/*",
    ".*/*",
    "*/.*/*",
    "DiligentSamples/*/assets/*",
    "*/Tests/*",
    "*/__pycache__/*"
]

def should_exclude(rel_dir):
    """
    Return True if the given relative directory (relative to the project root)
    should be excluded based on explicit directories, additional patterns,
    or if any directory component starts with a dot.
    """
    # Normalize to forward slashes.
    norm_rel_dir = rel_dir.replace(os.sep, '/')
    
    # Check explicit exclusions.
    for ex in EXCLUDED_DIRS:
        if norm_rel_dir == ex or norm_rel_dir.startswith(ex + '/'):
            return True

    # Check additional patterns.
    for pattern in ADDITIONAL_EXCLUDE_PATTERNS:
        if fnmatch.fnmatch(norm_rel_dir, pattern) or fnmatch.fnmatch(norm_rel_dir + '/', pattern):
            return True

    return False

def build_md_tree(root_dir, rel_dir=""):
    """
    Recursively build a tree structure starting at root_dir.
    
    Parameters:
      - root_dir: the absolute path to the project root.
      - rel_dir: the relative directory path from the root (default is "", meaning the root itself).
    
    Returns:
      A dictionary node with keys:
        "path": the relative path of the node,
        "files": a sorted list of Markdown file names in that directory,
        "subdirs": a list of child nodes (for subdirectories that contain Markdown files),
      or None if neither the current directory nor any of its subdirectories contain Markdown files.
    """
    current_path = os.path.join(root_dir, rel_dir)
    
    # Skip if the current relative directory should be excluded.
    if rel_dir and should_exclude(rel_dir):
        return None
    
    try:
        entries = os.listdir(current_path)
    except Exception as e:
        # If we cannot list the directory, skip it.
        return None
    
    files = []
    subdirs = []
    for entry in entries:
        full_path = os.path.join(current_path, entry)
        if os.path.isfile(full_path) and entry.lower().endswith('.md'):
            files.append(entry)
        elif os.path.isdir(full_path):
            subdirs.append(entry)
    
    # Recursively build nodes for subdirectories.
    children = []
    for d in sorted(subdirs):
        child_rel = os.path.join(rel_dir, d) if rel_dir else d
        child_node = build_md_tree(root_dir, child_rel)
        if child_node is not None:
            children.append(child_node)
    
    # If there are no markdown files here and no children with markdown files, skip this directory.
    if not files and not children:
        return None
    
    return {
        "path": rel_dir,
        "files": sorted(files),
        "subdirs": children
    }


def clean_level1(name):
    """If a level-1 folder name starts with 'Diligent' (case-insensitive), remove that prefix."""
    if name.lower().startswith("diligent"):
        return name[len("Diligent"):]
    return name


def compute_container(node, root_dir, level = 0):
    """
    Recursively process the tree node and compute its container ID and page text.

    Rules:
      - If the node contains exactly one Markdown file and no subdirectories,
        then its page text is empty (the file's page will be used as a subpage)
        and the function returns that file's page ID.
      - Otherwise, a container page is generated. Its page text is a block
        that starts with a \page definition (using a synthetic container ID and
        a title) and then lists as subpages all container IDs returned from:
            * Each Markdown file in the node
            * Each child node (from recursive invocation)
    The container ID is stored in node["container_id"] and the page text in node["page_text"].
    The function returns the container ID.
    """
    files = node.get("files", [])
    children = node.get("subdirs", [])
    rel_path = node.get("path", "")  # relative path from the project root

    # Case 1: Leaf node (exactly one Markdown file and no subdirectories).
    if len(files) == 1 and not children:
        md_file = files[0]
        full_path = os.path.join(root_dir, rel_path, md_file) if rel_path else os.path.join(root_dir, md_file)
        file_id = compute_page_id(root_dir, full_path)
        node["container_id"] = file_id
        node["page_text"] = ""  # no container page text for a single-file node
        return file_id

    # Case 2: Container node.
    # Create a synthetic container ID.
    if rel_path == "":
        container_id = "root"
        title = "User Guide"
    else:
        container_id = "dir_" + rel_path.replace(os.sep, "_")
        title = os.path.basename(rel_path)
        if level == 1:
            title = clean_level1(title)
    node["container_id"] = container_id

    # Gather subpage IDs from local Markdown files.
    subpage_ids = []
    for f in files:
        full_path = os.path.join(root_dir, rel_path, f) if rel_path else os.path.join(root_dir, f)
        file_id = compute_page_id(root_dir, full_path)
        subpage_ids.append(file_id)

    # Recurse into each child node.
    for child in children:
        child_id = compute_container(child, root_dir, level + 1)
        subpage_ids.append(child_id)

    # Construct the page text for this container node.
    # It defines a page with the container ID and title, and lists subpages.
    lines = []
    lines.append("/**\n")
    lines.append(f" * \\page {container_id} {title}\n")
    for sp in subpage_ids:
        lines.append(f" *\n")
        lines.append(f" * \\subpage {sp}\n")
    lines.append(" */\n\n")
    page_text = "".join(lines)
    node["page_text"] = page_text

    return container_id

def write_pages(node, out_stream):
    """
    Recursively traverse the tree and write out the page text for each node,
    if non-empty.
    """
    if node.get("page_text"):
        out_stream.write(node["page_text"])
    for child in node.get("subdirs", []):
        write_pages(child, out_stream)
        

def main():
    project_root = get_project_root()
    output_file = sys.argv[1] if len(sys.argv) >= 2 else None
    
    tree = build_md_tree(project_root)
    if tree is None:
        sys.stderr.write("No Markdown files found in the project.\n")
        return

    #print(json.dumps(tree, indent=4))

    compute_container(tree, project_root)
    
    if output_file:
       out_stream = open(output_file, 'w', encoding='utf-8')
    else:
       out_stream = sys.stdout
    write_pages(tree, out_stream)
        
if __name__ == "__main__":
    main()
